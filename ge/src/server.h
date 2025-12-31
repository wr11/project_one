#ifndef GEARENGINE_SERVER_H
#define GEARENGINE_SERVER_H

#include <uv.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <bson/bson.h>

// Client connection structure
typedef struct {
    uv_tcp_t handle;
    uv_write_t write_req;
    char buffer[4096];
    int buffer_len;
    struct server_context* server;
} client_t;

// Server context structure
typedef struct server_context {
    uv_loop_t* loop;
    uv_tcp_t server_handle;
    lua_State* L;
    int port;
    char* script_path;
    client_t** clients;
    int client_count;
    int client_capacity;
} server_context_t;

// Server functions
int server_init(server_context_t* ctx, int port, const char* script_path);
int server_start(server_context_t* ctx);
void server_stop(server_context_t* ctx);
void server_cleanup(server_context_t* ctx);

// Lua binding functions
void register_lua_bindings(lua_State* L, server_context_t* ctx);

// BSON handling functions
bson_t* bson_from_buffer(const char* buffer, int len);
int bson_to_buffer(bson_t* bson, char* buffer, int buffer_size);

#endif // GEARENGINE_SERVER_H
