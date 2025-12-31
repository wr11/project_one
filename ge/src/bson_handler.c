#include "server.h"
#include <string.h>
#include <stdlib.h>

// Create BSON document from buffer
bson_t* bson_from_buffer(const char* buffer, int len) {
    if (len < 5) {  // Minimum BSON size
        return NULL;
    }
    
    bson_t* bson = bson_new_from_data((const uint8_t*)buffer, len);
    if (!bson) {
        return NULL;
    }
    
    // Validate BSON document
    if (!bson_validate(bson, BSON_VALIDATE_NONE, NULL)) {
        bson_destroy(bson);
        return NULL;
    }
    
    return bson;
}

// Convert BSON document to buffer
int bson_to_buffer(bson_t* bson, char* buffer, int buffer_size) {
    if (!bson || !buffer || buffer_size <= 0) {
        return -1;
    }
    
    uint32_t len = 0;
    const uint8_t* data = bson_get_data(bson);
    
    // Read BSON length (first 4 bytes)
    memcpy(&len, data, sizeof(uint32_t));
    
    if (len > (uint32_t)buffer_size) {
        return -1;
    }
    
    memcpy(buffer, data, len);
    return len;
}

// Create BSON document from Lua table (helper function)
bson_t* bson_from_lua_table(lua_State* L, int index) {
    bson_t* bson = bson_new();
    bson_t child;
    
    if (!lua_istable(L, index)) {
        bson_destroy(bson);
        return NULL;
    }
    
    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        const char* key = lua_tostring(L, -2);
        int type = lua_type(L, -1);
        
        switch (type) {
            case LUA_TNUMBER:
                if (lua_isinteger(L, -1)) {
                    BSON_APPEND_INT64(bson, key, lua_tointeger(L, -1));
                } else {
                    BSON_APPEND_DOUBLE(bson, key, lua_tonumber(L, -1));
                }
                break;
            case LUA_TSTRING:
                BSON_APPEND_UTF8(bson, key, lua_tostring(L, -1));
                break;
            case LUA_TBOOLEAN:
                BSON_APPEND_BOOL(bson, key, lua_toboolean(L, -1));
                break;
            case LUA_TTABLE:
                BSON_APPEND_DOCUMENT_BEGIN(bson, key, &child);
                // Recursively process nested table (simplified version)
                // Note: No BSON_APPEND_DOCUMENT_END macro, use function directly
                bson_append_document_end(bson, &child);
                break;
        }
        
        lua_pop(L, 1);
    }
    
    return bson;
}
