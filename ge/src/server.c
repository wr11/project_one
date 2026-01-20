#include "server.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

// Configuration constants
#define IDLE_TIMEOUT_MS 300000   // 5 minutes idle timeout

// Forward declarations
static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
void on_client_close(uv_handle_t* handle);  // Non-static, used by lua_bindings.c
static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
static void on_idle_timeout(uv_timer_t* timer);
static void process_message(client_t* client, const char* data, size_t len);
static int is_client_valid(client_t* client);

// Helper: Check if client is valid
static int is_client_valid(client_t* client) {
    return client != NULL &&
           client->magic == CLIENT_MAGIC &&
           client->is_valid;
}

// Helper: Mark client as invalid
static void invalidate_client(client_t* client) {
    if (client) {
        client->is_valid = 0;
        client->magic = 0;
    }
}

// Helper: Update client activity timestamp
static void update_client_activity(client_t* client) {
    if (is_client_valid(client) && client->server) {
        client->last_activity = uv_now(client->server->loop);
    }
}

// Idle timeout callback
static void on_idle_timeout(uv_timer_t* timer) {
    client_t* client = (client_t*)timer->data;
    if (!is_client_valid(client)) {
        return;
    }

    server_context_t* ctx = client->server;
    uint64_t now = uv_now(ctx->loop);

    if (now - client->last_activity >= IDLE_TIMEOUT_MS) {
        fprintf(stderr, "Client timeout, closing connection\n");

        // Stop timer first
        uv_timer_stop(&client->idle_timer);

        // Close connection
        if (!uv_is_closing((uv_handle_t*)&client->handle)) {
            uv_close((uv_handle_t*)&client->handle, on_client_close);
        }
    }
}

// New client connection callback
static void on_new_connection(uv_stream_t* server, int status) {
    if (status < 0) {
        fprintf(stderr, "New connection error: %s\n", uv_strerror(status));
        return;
    }

    server_context_t* ctx = (server_context_t*)server->data;

    // Allocate client structure
    client_t* client = (client_t*)malloc(sizeof(client_t));
    if (!client) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    memset(client, 0, sizeof(client_t));
    client->server = ctx;
    client->magic = CLIENT_MAGIC;
    client->is_valid = 1;
    client->last_activity = uv_now(ctx->loop);

    // Initialize TCP handle
    if (uv_tcp_init(ctx->loop, &client->handle) != 0) {
        fprintf(stderr, "uv_tcp_init failed\n");
        free(client);
        return;
    }

    // Accept connection
    if (uv_accept(server, (uv_stream_t*)&client->handle) == 0) {
        // Expand client array if needed
        if (ctx->client_count >= ctx->client_capacity) {
            int new_capacity = ctx->client_capacity ? ctx->client_capacity * 2 : INITIAL_CLIENT_CAPACITY;
            client_t** new_clients = (client_t**)realloc(ctx->clients,
                new_capacity * sizeof(client_t*));
            if (!new_clients) {
                fprintf(stderr, "Client list realloc failed\n");
                invalidate_client(client);
                uv_close((uv_handle_t*)&client->handle, on_client_close);
                return;
            }
            ctx->clients = new_clients;
            ctx->client_capacity = new_capacity;
        }

        // Add to client array and set index
        client->array_index = ctx->client_count;
        ctx->clients[ctx->client_count++] = client;

        // Set client data
        client->handle.data = client;

        // Initialize idle timeout timer
        if (uv_timer_init(ctx->loop, &client->idle_timer) != 0) {
            fprintf(stderr, "uv_timer_init failed\n");
            ctx->client_count--;
            invalidate_client(client);
            uv_close((uv_handle_t*)&client->handle, on_client_close);
            return;
        }
        client->idle_timer.data = client;

        // Start idle timer (check every 60 seconds)
        if (uv_timer_start(&client->idle_timer, on_idle_timeout, 60000, 60000) != 0) {
            fprintf(stderr, "uv_timer_start failed\n");
            ctx->client_count--;
            invalidate_client(client);
            // Close both timer and handle
            uv_close((uv_handle_t*)&client->idle_timer, NULL);
            uv_close((uv_handle_t*)&client->handle, on_client_close);
            return;
        }

        // Start reading data
        if (uv_read_start((uv_stream_t*)&client->handle,
                alloc_buffer,
                on_read) != 0) {
            fprintf(stderr, "uv_read_start failed\n");
            ctx->client_count--;
            uv_timer_stop(&client->idle_timer);
            invalidate_client(client);
            // Close both timer and handle
            uv_close((uv_handle_t*)&client->idle_timer, NULL);
            uv_close((uv_handle_t*)&client->handle, on_client_close);
            return;
        }

        // Call Lua callback
        lua_getglobal(ctx->L, "on_client_connect");
        if (lua_isfunction(ctx->L, -1)) {
            lua_pushlightuserdata(ctx->L, client);
            if (lua_pcall(ctx->L, 1, 0, 0) != LUA_OK) {
                fprintf(stderr, "Lua error: %s\n", lua_tostring(ctx->L, -1));
                lua_pop(ctx->L, 1);
            }
        } else {
            lua_pop(ctx->L, 1);
        }
    } else {
        invalidate_client(client);
        uv_close((uv_handle_t*)&client->handle, on_client_close);
    }
}

// Process a complete message
static void process_message(client_t* client, const char* data, size_t len) {
    if (!is_client_valid(client)) {
        return;
    }

    server_context_t* ctx = client->server;

    // Update activity timestamp
    update_client_activity(client);

    // Call Lua callback to process data
    lua_getglobal(ctx->L, "on_client_data");
    if (lua_isfunction(ctx->L, -1)) {
        lua_pushlightuserdata(ctx->L, client);
        lua_pushlstring(ctx->L, data, len);
        if (lua_pcall(ctx->L, 2, 0, 0) != LUA_OK) {
            fprintf(stderr, "Lua error: %s\n", lua_tostring(ctx->L, -1));
            lua_pop(ctx->L, 1);
        }
    } else {
        lua_pop(ctx->L, 1);
    }
}

// Read data callback with message boundary handling
static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    client_t* client = (client_t*)stream->data;

    if (!is_client_valid(client)) {
        free(buf->base);
        return;
    }

    server_context_t* ctx = client->server;

    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error: %s\n", uv_strerror(nread));
        }
        invalidate_client(client);
        uv_timer_stop(&client->idle_timer);
        uv_close((uv_handle_t*)stream, on_client_close);
        free(buf->base);
        return;
    }

    if (nread == 0) {
        free(buf->base);
        return;
    }

    // Update activity
    update_client_activity(client);

    // Process incoming data with message boundary handling
    size_t offset = 0;
    while (offset < (size_t)nread) {
        size_t remaining = (size_t)nread - offset;

        // If we don't have a complete header yet, accumulate data
        if (client->buffer_used < MESSAGE_HEADER_SIZE) {
            size_t to_copy = MESSAGE_HEADER_SIZE - client->buffer_used;
            if (to_copy > remaining) {
                to_copy = remaining;
            }

            memcpy(client->buffer + client->buffer_used, buf->base + offset, to_copy);
            client->buffer_used += to_copy;
            offset += to_copy;

            // Check if we now have a complete header
            if (client->buffer_used == MESSAGE_HEADER_SIZE) {
                // Read length (network byte order - big-endian 32-bit integer)
                uint32_t msg_len_net;
                memcpy(&msg_len_net, client->buffer, sizeof(uint32_t));
                uint32_t msg_len = ntohl(msg_len_net);  // Convert from network to host byte order

                // Validate message length
                if (msg_len == 0 || msg_len > CLIENT_BUFFER_SIZE - MESSAGE_HEADER_SIZE) {
                    fprintf(stderr, "Invalid message length: %u\n", msg_len);
                    invalidate_client(client);
                    uv_timer_stop(&client->idle_timer);
                    uv_close((uv_handle_t*)stream, on_client_close);
                    free(buf->base);
                    return;
                }

                client->expected_length = (size_t)msg_len;
            }
            continue;
        }

        // We have a header, now accumulate message body
        size_t total_expected = MESSAGE_HEADER_SIZE + client->expected_length;
        size_t to_copy = total_expected - client->buffer_used;
        if (to_copy > remaining) {
            to_copy = remaining;
        }

        memcpy(client->buffer + client->buffer_used, buf->base + offset, to_copy);
        client->buffer_used += to_copy;
        offset += to_copy;

        // Check if we have a complete message
        if (client->buffer_used == total_expected) {
            // Process the message (skip the 4-byte header)
            process_message(client, client->buffer + MESSAGE_HEADER_SIZE, client->expected_length);

            // Reset for next message
            client->buffer_used = 0;
            client->expected_length = 0;
        }
    }

    free(buf->base);
}

// Client close callback
void on_client_close(uv_handle_t* handle) {
    client_t* client = (client_t*)handle->data;

    if (!client) {
        return;
    }

    server_context_t* ctx = client->server;

    // Call Lua callback before invalidating (Lua may still need to access client data)
    if (client->magic == CLIENT_MAGIC) {
        lua_getglobal(ctx->L, "on_client_disconnect");
        if (lua_isfunction(ctx->L, -1)) {
            lua_pushlightuserdata(ctx->L, client);
            if (lua_pcall(ctx->L, 1, 0, 0) != LUA_OK) {
                fprintf(stderr, "Lua error: %s\n", lua_tostring(ctx->L, -1));
                lua_pop(ctx->L, 1);
            }
        } else {
            lua_pop(ctx->L, 1);
        }
    }

    // Remove from client list using O(1) swap-with-last
    if (client->array_index >= 0 && client->array_index < ctx->client_count) {
        int idx = client->array_index;
        int last_idx = ctx->client_count - 1;

        if (idx != last_idx) {
            // Swap with last element
            ctx->clients[idx] = ctx->clients[last_idx];
            // Update the swapped client's index
            ctx->clients[idx]->array_index = idx;
        }

        ctx->client_count--;
    }

    // Invalidate client
    invalidate_client(client);

    // Close timer if not already closing
    if (!uv_is_closing((uv_handle_t*)&client->idle_timer)) {
        uv_close((uv_handle_t*)&client->idle_timer, NULL);
    }

    // Free client structure
    free(client);
}

// Allocate buffer callback - always use heap allocation for consistency
static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    (void)handle;  // Unused parameter

    // Use a reasonable buffer size (64KB for better performance)
    size_t alloc_size = suggested_size > 65536 ? suggested_size : 65536;

    buf->base = (char*)malloc(alloc_size);
    if (!buf->base) {
        buf->len = 0;
        return;
    }
    buf->len = alloc_size;
}

// Initialize server
int server_init(server_context_t* ctx, int port, const char* script_path,
                const char* process_name, const char* config_path) {
    memset(ctx, 0, sizeof(server_context_t));

    ctx->port = port;

    // Duplicate strings and check for allocation failures
    if (script_path) {
        ctx->script_path = strdup(script_path);
        if (!ctx->script_path) {
            fprintf(stderr, "Failed to allocate memory for script_path\n");
            return -1;
        }
    }

    if (process_name) {
        ctx->process_name = strdup(process_name);
        if (!ctx->process_name) {
            fprintf(stderr, "Failed to allocate memory for process_name\n");
            server_cleanup(ctx);
            return -1;
        }
    }

    if (config_path) {
        ctx->config_path = strdup(config_path);
        if (!ctx->config_path) {
            fprintf(stderr, "Failed to allocate memory for config_path\n");
            server_cleanup(ctx);
            return -1;
        }
    }

    // Create event loop
    ctx->loop = uv_default_loop();
    
    // Initialize Lua state
    ctx->L = luaL_newstate();
    if (!ctx->L) {
        fprintf(stderr, "Failed to create Lua state\n");
        return -1;
    }
    
    // Load Lua standard libraries
    luaL_openlibs(ctx->L);
    
    // Register custom bindings
    register_lua_bindings(ctx->L, ctx);
    
    // Load script file
    if (script_path) {
        if (luaL_loadfile(ctx->L, script_path) != LUA_OK) {
            fprintf(stderr, "Failed to load script file: %s\n", lua_tostring(ctx->L, -1));
            lua_pop(ctx->L, 1);
            server_cleanup(ctx);
            return -1;
        }
        if (lua_pcall(ctx->L, 0, 0, 0) != LUA_OK) {
            fprintf(stderr, "Script execution error: %s\n", lua_tostring(ctx->L, -1));
            lua_pop(ctx->L, 1);
            server_cleanup(ctx);
            return -1;
        }
    }
    
    // Initialize TCP server
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", port, &addr);
    
    if (uv_tcp_init(ctx->loop, &ctx->server_handle) != 0) {
        fprintf(stderr, "Failed to init server handle\n");
        server_cleanup(ctx);
        return -1;
    }
    ctx->server_handle.data = ctx;
    
    if (uv_tcp_bind(&ctx->server_handle, (const struct sockaddr*)&addr, 0)) {
        fprintf(stderr, "Failed to bind address\n");
        server_cleanup(ctx);
        return -1;
    }
    
    return 0;
}

// Start server
int server_start(server_context_t* ctx) {
    int r = uv_listen((uv_stream_t*)&ctx->server_handle, LISTEN_BACKLOG, on_new_connection);
    if (r) {
        fprintf(stderr, "Listen failed: %s\n", uv_strerror(r));
        return -1;
    }
    
    printf("Server started on port %d\n", ctx->port);
    
    // Call Lua initialization callback
    lua_getglobal(ctx->L, "on_server_start");
    if (lua_isfunction(ctx->L, -1)) {
        if (lua_pcall(ctx->L, 0, 0, 0) != LUA_OK) {
            fprintf(stderr, "Lua error: %s\n", lua_tostring(ctx->L, -1));
            lua_pop(ctx->L, 1);
        }
    } else {
        lua_pop(ctx->L, 1);
    }
    
    // Run event loop
    return uv_run(ctx->loop, UV_RUN_DEFAULT);
}

// Server handle close callback
static void on_server_close(uv_handle_t* handle) {
    // Server handle closed, no additional cleanup needed here
    // The actual cleanup happens in server_cleanup()
}

// Stop server
void server_stop(server_context_t* ctx) {
    if (!uv_is_closing((uv_handle_t*)&ctx->server_handle)) {
        uv_close((uv_handle_t*)&ctx->server_handle, on_server_close);
    }

    // Close all client connections
    for (int i = 0; i < ctx->client_count; i++) {
        client_t* client = ctx->clients[i];
        if (is_client_valid(client)) {
            invalidate_client(client);
            uv_timer_stop(&client->idle_timer);
            uv_close((uv_handle_t*)&client->handle, on_client_close);
        }
    }
    
    // Call Lua cleanup callback
    lua_getglobal(ctx->L, "on_server_stop");
    if (lua_isfunction(ctx->L, -1)) {
        if (lua_pcall(ctx->L, 0, 0, 0) != LUA_OK) {
            fprintf(stderr, "Lua error: %s\n", lua_tostring(ctx->L, -1));
            lua_pop(ctx->L, 1);
        }
    } else {
        lua_pop(ctx->L, 1);
    }
}

// Cleanup server
void server_cleanup(server_context_t* ctx) {
    if (ctx->L) {
        logger_shutdown();
        lua_close(ctx->L);
        ctx->L = NULL;
    }
    
    if (ctx->clients) {
        free(ctx->clients);
        ctx->clients = NULL;
    }
    
    if (ctx->script_path) {
        free(ctx->script_path);
        ctx->script_path = NULL;
    }

    if (ctx->process_name) {
        free(ctx->process_name);
        ctx->process_name = NULL;
    }

    if (ctx->config_path) {
        free(ctx->config_path);
        ctx->config_path = NULL;
    }
}
