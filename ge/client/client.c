#include "client.h"
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
static void on_tty_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
static void on_connect(uv_connect_t* req, int status);
static void on_write(uv_write_t* req, int status);

// Allocate buffer callback
static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = (char*)malloc(suggested_size);
    buf->len = suggested_size;
}

// Read data callback
static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    client_context_t* ctx = (client_context_t*)stream->data;
    
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "Read error: %s\n", uv_strerror(nread));
        }
        client_disconnect(ctx);
        free(buf->base);
        return;
    }
    
    if (nread == 0) {
        free(buf->base);
        return;
    }
    
    // Call Lua callback to process received data
    lua_getglobal(ctx->L, "on_server_message");
    if (lua_isfunction(ctx->L, -1)) {
        lua_pushlstring(ctx->L, buf->base, nread);
        if (lua_pcall(ctx->L, 1, 0, 0) != LUA_OK) {
            fprintf(stderr, "Lua error: %s\n", lua_tostring(ctx->L, -1));
            lua_pop(ctx->L, 1);
        }
    } else {
        lua_pop(ctx->L, 1);
        // Default: print message
        printf("%.*s", (int)nread, buf->base);
    }
    
    free(buf->base);
}

// TTY read callback (stdin input)
static void on_tty_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    client_context_t* ctx = (client_context_t*)stream->data;
    
    if (nread < 0) {
        if (nread != UV_EOF) {
            fprintf(stderr, "TTY read error: %s\n", uv_strerror(nread));
        }
        free(buf->base);
        return;
    }
    
    if (nread == 0) {
        free(buf->base);
        return;
    }
    
    // Remove trailing newline
    if (nread > 0 && buf->base[nread-1] == '\n') {
        nread--;
    }
    if (nread > 0 && buf->base[nread-1] == '\r') {
        nread--;
    }
    
    if (nread > 0) {
        // Null-terminate the string
        buf->base[nread] = '\0';
        
        // Call Lua callback to handle input
        lua_getglobal(ctx->L, "on_user_input");
        if (lua_isfunction(ctx->L, -1)) {
            lua_pushlstring(ctx->L, buf->base, nread);
            if (lua_pcall(ctx->L, 1, 0, 0) != LUA_OK) {
                fprintf(stderr, "Lua error: %s\n", lua_tostring(ctx->L, -1));
                lua_pop(ctx->L, 1);
            }
        } else {
            lua_pop(ctx->L, 1);
            // Default: send message
            client_send(ctx, buf->base, nread);
            client_send(ctx, "\n", 1);
        }
    }
    
    free(buf->base);
}

// Connect callback
static void on_connect(uv_connect_t* req, int status) {
    client_context_t* ctx = (client_context_t*)req->data;
    
    if (status < 0) {
        fprintf(stderr, "Connection error: %s\n", uv_strerror(status));
        ctx->connected = 0;
        return;
    }
    
    ctx->connected = 1;
    printf("Connected to server %s:%d\n", ctx->host, ctx->port);
    
    // Start reading from socket
    uv_read_start((uv_stream_t*)&ctx->socket, alloc_buffer, on_read);
    
    // Start reading from stdin (TTY)
    uv_read_start((uv_stream_t*)&ctx->tty, alloc_buffer, on_tty_read);
    
    // Call Lua callback
    lua_getglobal(ctx->L, "on_connected");
    if (lua_isfunction(ctx->L, -1)) {
        if (lua_pcall(ctx->L, 0, 0, 0) != LUA_OK) {
            fprintf(stderr, "Lua error: %s\n", lua_tostring(ctx->L, -1));
            lua_pop(ctx->L, 1);
        }
    } else {
        lua_pop(ctx->L, 1);
    }
}

// Write callback
static void on_write(uv_write_t* req, int status) {
    if (status < 0) {
        fprintf(stderr, "Write error: %s\n", uv_strerror(status));
    }
    free(req);
}

// Initialize client
int client_init(client_context_t* ctx, const char* host, int port, const char* script_path) {
    memset(ctx, 0, sizeof(client_context_t));
    
    ctx->host = host ? strdup(host) : strdup("localhost");
    ctx->port = port;
    ctx->script_path = script_path ? strdup(script_path) : NULL;
    ctx->connected = 0;
    
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
    register_client_lua_bindings(ctx->L, ctx);
    
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
    
    // Initialize TCP socket
    uv_tcp_init(ctx->loop, &ctx->socket);
    ctx->socket.data = ctx;
    ctx->connect_req.data = ctx;
    
    // Initialize TTY for stdin (non-blocking input)
    uv_tty_init(ctx->loop, &ctx->tty, 0, 1);
    ctx->tty.data = ctx;
    uv_tty_set_mode(&ctx->tty, UV_TTY_MODE_NORMAL);
    
    return 0;
}

// Connect to server
int client_connect(client_context_t* ctx) {
    struct sockaddr_in addr;
    uv_ip4_addr(ctx->host, ctx->port, &addr);
    
    return uv_tcp_connect(&ctx->connect_req, &ctx->socket, 
                          (const struct sockaddr*)&addr, on_connect);
}

// Send data to server
int client_send(client_context_t* ctx, const char* data, size_t len) {
    if (!ctx->connected) {
        return -1;
    }
    
    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    uv_buf_t buf = uv_buf_init((char*)data, len);
    
    return uv_write(req, (uv_stream_t*)&ctx->socket, &buf, 1, on_write);
}

// Disconnect from server
void client_disconnect(client_context_t* ctx) {
    if (ctx->connected) {
        ctx->connected = 0;
        
        // Call Lua callback
        lua_getglobal(ctx->L, "on_disconnected");
        if (lua_isfunction(ctx->L, -1)) {
            if (lua_pcall(ctx->L, 0, 0, 0) != LUA_OK) {
                fprintf(stderr, "Lua error: %s\n", lua_tostring(ctx->L, -1));
                lua_pop(ctx->L, 1);
            }
        } else {
            lua_pop(ctx->L, 1);
        }
        
        uv_close((uv_handle_t*)&ctx->socket, NULL);
        uv_close((uv_handle_t*)&ctx->tty, NULL);
        printf("Disconnected from server\n");
    }
}

// Cleanup client
void client_cleanup(client_context_t* ctx) {
    if (ctx->connected) {
        client_disconnect(ctx);
    }
    
    if (ctx->L) {
        lua_close(ctx->L);
        ctx->L = NULL;
    }
    
    if (ctx->host) {
        free(ctx->host);
        ctx->host = NULL;
    }
    
    if (ctx->script_path) {
        free(ctx->script_path);
        ctx->script_path = NULL;
    }
}
