#include "server.h"
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

// Forward declarations
static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
static void on_client_close(uv_handle_t* handle);
static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);

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
    
    // Initialize TCP handle
    uv_tcp_init(ctx->loop, &client->handle);
    
    // Accept connection
    if (uv_accept(server, (uv_stream_t*)&client->handle) == 0) {
        // Expand client array
        if (ctx->client_count >= ctx->client_capacity) {
            ctx->client_capacity = ctx->client_capacity ? ctx->client_capacity * 2 : 16;
            ctx->clients = (client_t**)realloc(ctx->clients, 
                ctx->client_capacity * sizeof(client_t*));
        }
        ctx->clients[ctx->client_count++] = client;
        
        // Set client data
        client->handle.data = client;
        
        // Start reading data
        uv_read_start((uv_stream_t*)&client->handle, 
            alloc_buffer, 
            on_read);
        
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
        uv_close((uv_handle_t*)&client->handle, NULL);
        free(client);
    }
}

// Read data callback
static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    client_t* client = (client_t*)stream->data;
    server_context_t* ctx = client->server;
    
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error: %s\n", uv_strerror(nread));
        }
        uv_close((uv_handle_t*)stream, on_client_close);
        free(buf->base);
        return;
    }
    
    if (nread == 0) {
        free(buf->base);
        return;
    }
    
    // Call Lua callback to process data
    lua_getglobal(ctx->L, "on_client_data");
    if (lua_isfunction(ctx->L, -1)) {
        lua_pushlightuserdata(ctx->L, client);
        lua_pushlstring(ctx->L, buf->base, nread);
        if (lua_pcall(ctx->L, 2, 0, 0) != LUA_OK) {
            fprintf(stderr, "Lua error: %s\n", lua_tostring(ctx->L, -1));
            lua_pop(ctx->L, 1);
        }
    } else {
        lua_pop(ctx->L, 1);
    }
    
    free(buf->base);
}

// Client close callback
static void on_client_close(uv_handle_t* handle) {
    client_t* client = (client_t*)handle->data;
    server_context_t* ctx = client->server;
    
    // Remove from client list
    for (int i = 0; i < ctx->client_count; i++) {
        if (ctx->clients[i] == client) {
            ctx->clients[i] = ctx->clients[ctx->client_count - 1];
            ctx->client_count--;
            break;
        }
    }
    
    // Call Lua callback
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
    
    free(client);
}

// Allocate buffer callback
static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = (char*)malloc(suggested_size);
    buf->len = suggested_size;
}

// Initialize server
int server_init(server_context_t* ctx, int port, const char* script_path) {
    memset(ctx, 0, sizeof(server_context_t));
    
    ctx->port = port;
    ctx->script_path = script_path ? strdup(script_path) : NULL;
    
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
            return -1;
        }
        if (lua_pcall(ctx->L, 0, 0, 0) != LUA_OK) {
            fprintf(stderr, "Script execution error: %s\n", lua_tostring(ctx->L, -1));
            lua_pop(ctx->L, 1);
            return -1;
        }
    }
    
    // Initialize TCP server
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", port, &addr);
    
    uv_tcp_init(ctx->loop, &ctx->server_handle);
    ctx->server_handle.data = ctx;
    
    if (uv_tcp_bind(&ctx->server_handle, (const struct sockaddr*)&addr, 0)) {
        fprintf(stderr, "Failed to bind address\n");
        return -1;
    }
    
    return 0;
}

// Start server
int server_start(server_context_t* ctx) {
    int r = uv_listen((uv_stream_t*)&ctx->server_handle, 128, on_new_connection);
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

// Stop server
void server_stop(server_context_t* ctx) {
    uv_close((uv_handle_t*)&ctx->server_handle, NULL);
    
    // Close all client connections
    for (int i = 0; i < ctx->client_count; i++) {
        uv_close((uv_handle_t*)&ctx->clients[i]->handle, on_client_close);
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
}
