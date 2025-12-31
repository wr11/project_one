#ifndef GEARENGINE_CLIENT_H
#define GEARENGINE_CLIENT_H

#include <uv.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Client context structure
typedef struct {
    uv_loop_t* loop;
    uv_tcp_t socket;
    uv_connect_t connect_req;
    uv_tty_t tty;
    lua_State* L;
    char* host;
    int port;
    char* script_path;
    int connected;
} client_context_t;

// Client functions
int client_init(client_context_t* ctx, const char* host, int port, const char* script_path);
int client_connect(client_context_t* ctx);
void client_disconnect(client_context_t* ctx);
void client_cleanup(client_context_t* ctx);
int client_send(client_context_t* ctx, const char* data, size_t len);

// Lua binding functions
void register_client_lua_bindings(lua_State* L, client_context_t* ctx);

#endif // GEARENGINE_CLIENT_H
