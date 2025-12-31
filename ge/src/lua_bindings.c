#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Send data to client (Lua binding)
static int lua_send_data(lua_State* L) {
    client_t* client = (client_t*)lua_touserdata(L, 1);
    size_t len;
    const char* data = luaL_checklstring(L, 2, &len);
    
    if (!client) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Invalid client");
        return 2;
    }
    
    uv_buf_t buf = uv_buf_init((char*)data, len);
    int r = uv_write(&client->write_req, (uv_stream_t*)&client->handle, 
                     &buf, 1, NULL);
    
    lua_pushboolean(L, r == 0);
    if (r != 0) {
        lua_pushstring(L, uv_strerror(r));
        return 2;
    }
    
    return 1;
}

// Send BSON data to client (Lua binding)
static int lua_send_bson(lua_State* L) {
    client_t* client = (client_t*)lua_touserdata(L, 1);
    size_t len;
    const char* bson_data = luaL_checklstring(L, 2, &len);
    
    if (!client) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Invalid client");
        return 2;
    }
    
    // Send BSON data
    uv_buf_t buf = uv_buf_init((char*)bson_data, len);
    int r = uv_write(&client->write_req, (uv_stream_t*)&client->handle, 
                     &buf, 1, NULL);
    
    lua_pushboolean(L, r == 0);
    if (r != 0) {
        lua_pushstring(L, uv_strerror(r));
        return 2;
    }
    
    return 1;
}

// Close client connection (Lua binding)
static int lua_close_client(lua_State* L) {
    client_t* client = (client_t*)lua_touserdata(L, 1);
    
    if (!client) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Invalid client");
        return 2;
    }
    
    uv_close((uv_handle_t*)&client->handle, NULL);
    lua_pushboolean(L, 1);
    return 1;
}

// Get client count (Lua binding)
static int lua_get_client_count(lua_State* L) {
    server_context_t* ctx = (server_context_t*)lua_touserdata(L, lua_upvalueindex(1));
    lua_pushinteger(L, ctx->client_count);
    return 1;
}

// Print log (Lua binding)
static int lua_log(lua_State* L) {
    const char* msg = luaL_checkstring(L, 1);
    printf("[Lua] %s\n", msg);
    return 0;
}

// Register all Lua bindings
void register_lua_bindings(lua_State* L, server_context_t* ctx) {
    // Push server context as upvalue
    lua_pushlightuserdata(L, ctx);
    
    // Create gear table
    lua_newtable(L);
    
    // Register functions
    lua_pushcfunction(L, lua_send_data);
    lua_setfield(L, -2, "send_data");
    
    lua_pushcfunction(L, lua_send_bson);
    lua_setfield(L, -2, "send_bson");
    
    lua_pushcfunction(L, lua_close_client);
    lua_setfield(L, -2, "close_client");
    
    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, lua_get_client_count, 1);
    lua_setfield(L, -2, "get_client_count");
    
    lua_pushcfunction(L, lua_log);
    lua_setfield(L, -2, "log");
    
    // Set as global gear table
    lua_setglobal(L, "gear");
}
