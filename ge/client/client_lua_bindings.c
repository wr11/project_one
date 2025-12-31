#include "client.h"
#include <stdio.h>
#include <string.h>
#include <uv.h>

// Send data to server (Lua binding)
static int lua_send_data(lua_State* L) {
    client_context_t* ctx = (client_context_t*)lua_touserdata(L, lua_upvalueindex(1));
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    
    if (!ctx || !ctx->connected) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Not connected");
        return 2;
    }
    
    int r = client_send(ctx, data, len);
    
    lua_pushboolean(L, r == 0);
    if (r != 0) {
        lua_pushstring(L, uv_strerror(r));
        return 2;
    }
    
    return 1;
}

// Check connection status (Lua binding)
static int lua_is_connected(lua_State* L) {
    client_context_t* ctx = (client_context_t*)lua_touserdata(L, lua_upvalueindex(1));
    lua_pushboolean(L, ctx && ctx->connected);
    return 1;
}

// Disconnect from server (Lua binding)
static int lua_disconnect(lua_State* L) {
    client_context_t* ctx = (client_context_t*)lua_touserdata(L, lua_upvalueindex(1));
    if (ctx) {
        client_disconnect(ctx);
    }
    lua_pushboolean(L, 1);
    return 1;
}

// Print log (Lua binding)
static int lua_log(lua_State* L) {
    const char* msg = luaL_checkstring(L, 1);
    printf("[Client] %s\n", msg);
    return 0;
}

// Register all Lua bindings
void register_client_lua_bindings(lua_State* L, client_context_t* ctx) {
    // Push client context as upvalue
    lua_pushlightuserdata(L, ctx);
    
    // Create gear_client table
    lua_newtable(L);
    
    // Register functions
    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, lua_send_data, 1);
    lua_setfield(L, -2, "send");
    
    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, lua_is_connected, 1);
    lua_setfield(L, -2, "is_connected");
    
    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, lua_disconnect, 1);
    lua_setfield(L, -2, "disconnect");
    
    lua_pushcfunction(L, lua_log);
    lua_setfield(L, -2, "log");
    
    // Set as global gear_client table
    lua_setglobal(L, "gear_client");
}
