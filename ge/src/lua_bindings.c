#include "server.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
    char* data;
} write_req_t;

// Helper: Validate client pointer from Lua
static int is_client_valid_lua(client_t* client) {
    return client != NULL &&
           client->magic == CLIENT_MAGIC &&
           client->is_valid;
}

// Helper: Safe Lua function call with error handling
static int safe_lua_call(lua_State* L, const char* func_name, int nargs) {
    lua_getglobal(L, func_name);
    if (!lua_isfunction(L, -nargs - 1)) {
        lua_pop(L, nargs + 1);
        return -1;
    }

    // Move function before arguments
    if (nargs > 0) {
        lua_insert(L, -nargs - 1);
    }

    if (lua_pcall(L, nargs, 0, 0) != LUA_OK) {
        fprintf(stderr, "Lua error in %s: %s\n", func_name, lua_tostring(L, -1));
        lua_pop(L, 1);
        return -1;
    }

    return 0;
}

static void on_write_done(uv_write_t* req, int status) {
    write_req_t* wr = (write_req_t*)req;
    if (status < 0) {
        fprintf(stderr, "Write error: %s\n", uv_strerror(status));
    }
    free(wr->data);
    free(wr);
}

static int send_buffer(lua_State* L, client_t* client, const char* data, size_t len) {
    if (!is_client_valid_lua(client)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Invalid client");
        return 2;
    }
    if (uv_is_closing((uv_handle_t*)&client->handle)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Client is closing");
        return 2;
    }
    if (len > CLIENT_BUFFER_SIZE - MESSAGE_HEADER_SIZE) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Data too large");
        return 2;
    }
    if (len == 0) {
        lua_pushboolean(L, 1);
        return 1;
    }

    // Allocate buffer for header + data
    size_t total_len = MESSAGE_HEADER_SIZE + len;
    write_req_t* wr = (write_req_t*)malloc(sizeof(write_req_t));
    if (!wr) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Out of memory");
        return 2;
    }

    wr->data = (char*)malloc(total_len);
    if (!wr->data) {
        free(wr);
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Out of memory");
        return 2;
    }

    // Write length header (little-endian 32-bit)
    uint32_t msg_len = (uint32_t)len;
    memcpy(wr->data, &msg_len, sizeof(uint32_t));

    // Write message body
    memcpy(wr->data + MESSAGE_HEADER_SIZE, data, len);

    wr->buf = uv_buf_init(wr->data, (unsigned int)total_len);

    int r = uv_write(&wr->req, (uv_stream_t*)&client->handle, &wr->buf, 1, on_write_done);
    if (r != 0) {
        free(wr->data);
        free(wr);
        lua_pushboolean(L, 0);
        lua_pushstring(L, uv_strerror(r));
        return 2;
    }

    lua_pushboolean(L, 1);
    return 1;
}

// Send data to client (Lua binding)
static int lua_send_data(lua_State* L) {
    client_t* client = (client_t*)lua_touserdata(L, 1);
    size_t len;
    const char* data = luaL_checklstring(L, 2, &len);

    return send_buffer(L, client, data, len);
}

// Send BSON data to client (Lua binding)
static int lua_send_bson(lua_State* L) {
    client_t* client = (client_t*)lua_touserdata(L, 1);
    size_t len;
    const char* bson_data = luaL_checklstring(L, 2, &len);

    // Send BSON data
    return send_buffer(L, client, bson_data, len);
}

// Close client connection (Lua binding)
static int lua_close_client(lua_State* L) {
    client_t* client = (client_t*)lua_touserdata(L, 1);

    if (!is_client_valid_lua(client)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Invalid client");
        return 2;
    }

    if (!uv_is_closing((uv_handle_t*)&client->handle)) {
        // Mark as invalid before closing
        client->is_valid = 0;
        uv_timer_stop(&client->idle_timer);
        // Use proper close callback
        uv_close((uv_handle_t*)&client->handle, on_client_close);
    }
    lua_pushboolean(L, 1);
    return 1;
}

// Check if client is closing (Lua binding)
static int lua_is_closing_client(lua_State* L) {
    client_t* client = (client_t*)lua_touserdata(L, 1);
    if (!is_client_valid_lua(client)) {
        lua_pushboolean(L, 1);  // Invalid client is considered as closing
        return 1;
    }
    lua_pushboolean(L, uv_is_closing((uv_handle_t*)&client->handle));
    return 1;
}

// Get client count (Lua binding)
static int lua_get_client_count(lua_State* L) {
    server_context_t* ctx = (server_context_t*)lua_touserdata(L, lua_upvalueindex(1));
    lua_pushinteger(L, ctx->client_count);
    return 1;
}

// Helper: find last path separator (for config path building)
static char* find_last_path_sep(char* path) {
    char* slash = strrchr(path, '/');
    char* backslash = strrchr(path, '\\');
    if (!slash) {
        return backslash;
    }
    if (!backslash) {
        return slash;
    }
    return (slash > backslash) ? slash : backslash;
}

// Helper: get project root directory (for config path building)
static void get_project_root(char* buffer, size_t buffer_size) {
    char exe_path[1024];
    size_t exe_size = sizeof(exe_path);
    if (uv_exepath(exe_path, &exe_size) == 0) {
        exe_path[exe_size] = '\0';
        char* last_sep = find_last_path_sep(exe_path);
        if (last_sep) {
            *last_sep = '\0';
            char* parent_sep = find_last_path_sep(exe_path);
            if (parent_sep) {
                *parent_sep = '\0';
                strncpy(buffer, exe_path, buffer_size - 1);
                buffer[buffer_size - 1] = '\0';
                return;
            }
        }
    }

    size_t cwd_size = buffer_size;
    if (uv_cwd(buffer, &cwd_size) != 0) {
        strncpy(buffer, ".", buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
    }
}

// Read entire file into buffer
static int read_file_all(const char* path, char** out, size_t* out_len) {
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        return -1;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return -1;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }
    char* buffer = (char*)malloc((size_t)size + 1);
    if (!buffer) {
        fclose(fp);
        return -1;
    }
    size_t read_size = fread(buffer, 1, (size_t)size, fp);
    fclose(fp);
    if (read_size != (size_t)size) {
        free(buffer);
        return -1;
    }
    buffer[size] = '\0';
    *out = buffer;
    if (out_len) {
        *out_len = (size_t)size;
    }
    return 0;
}

// Build config file path
static int build_config_path(char* buffer, size_t buffer_size) {
#ifdef _WIN32
    const char sep = '\\';
#else
    const char sep = '/';
#endif
    char project_root[1024];
    get_project_root(project_root, sizeof(project_root));
    int n = snprintf(buffer, buffer_size, "%s%cserver%cconf%cconf.json",
                     project_root, sep, sep, sep);
    return (n > 0 && (size_t)n < buffer_size) ? 0 : -1;
}

// Extract number from BSON iterator
static int bson_iter_to_size(const bson_iter_t* iter, size_t* out) {
    if (BSON_ITER_HOLDS_INT32(iter)) {
        int32_t v = bson_iter_int32(iter);
        if (v > 0) {
            *out = (size_t)v;
            return 1;
        }
    } else if (BSON_ITER_HOLDS_INT64(iter)) {
        int64_t v = bson_iter_int64(iter);
        if (v > 0) {
            *out = (size_t)v;
            return 1;
        }
    } else if (BSON_ITER_HOLDS_DOUBLE(iter)) {
        double v = bson_iter_double(iter);
        if (v > 0) {
            *out = (size_t)v;
            return 1;
        }
    }
    return 0;
}

// Apply log queue settings from config
static void apply_log_queue_settings(const bson_t* doc) {
    bson_iter_t iter;
    if (!bson_iter_init_find(&iter, doc, "common") || !BSON_ITER_HOLDS_DOCUMENT(&iter)) {
        return;
    }
    const uint8_t* data = NULL;
    uint32_t len = 0;
    bson_iter_document(&iter, &len, &data);
    bson_t common;
    bson_init_static(&common, data, len);

    size_t log_queue_max = 10000;
    size_t log_queue_warn = 8000;
    
    bson_iter_t c_iter;
    if (bson_iter_init_find(&c_iter, &common, "log_queue_max")) {
        bson_iter_to_size(&c_iter, &log_queue_max);
    }
    if (bson_iter_init_find(&c_iter, &common, "log_queue_warn")) {
        bson_iter_to_size(&c_iter, &log_queue_warn);
    }
    
    // Apply to logger
    logger_set_queue_params(log_queue_max, log_queue_warn);
}

// Forward declaration
static void push_bson_value(lua_State* L, const bson_value_t* value);

// Push BSON document to Lua table
static void push_bson_document(lua_State* L, const bson_t* doc) {
    lua_newtable(L);
    bson_iter_t iter;
    if (bson_iter_init(&iter, doc)) {
        while (bson_iter_next(&iter)) {
            const char* key = bson_iter_key(&iter);
            lua_pushstring(L, key);
            push_bson_value(L, bson_iter_value(&iter));
            lua_settable(L, -3);
        }
    }
}

// Push BSON array to Lua table
static void push_bson_array(lua_State* L, const bson_t* arr) {
    lua_newtable(L);
    bson_iter_t iter;
    if (bson_iter_init(&iter, arr)) {
        int index = 1;
        while (bson_iter_next(&iter)) {
            push_bson_value(L, bson_iter_value(&iter));
            lua_rawseti(L, -2, index++);
        }
    }
}

// Push BSON value to Lua stack
static void push_bson_value(lua_State* L, const bson_value_t* value) {
    switch (value->value_type) {
        case BSON_TYPE_DOCUMENT: {
            bson_t child;
            bson_init_static(&child, value->value.v_doc.data, value->value.v_doc.data_len);
            push_bson_document(L, &child);
            break;
        }
        case BSON_TYPE_ARRAY: {
            bson_t child;
            bson_init_static(&child, value->value.v_doc.data, value->value.v_doc.data_len);
            push_bson_array(L, &child);
            break;
        }
        case BSON_TYPE_UTF8:
            lua_pushlstring(L, value->value.v_utf8.str, value->value.v_utf8.len);
            break;
        case BSON_TYPE_INT32:
            lua_pushinteger(L, value->value.v_int32);
            break;
        case BSON_TYPE_INT64:
            lua_pushinteger(L, (lua_Integer)value->value.v_int64);
            break;
        case BSON_TYPE_DOUBLE:
            lua_pushnumber(L, value->value.v_double);
            break;
        case BSON_TYPE_BOOL:
            lua_pushboolean(L, value->value.v_bool);
            break;
        case BSON_TYPE_NULL:
            lua_pushnil(L);
            break;
        default:
            lua_pushnil(L);
            break;
    }
}

// Load config and push to Lua stack, returns 1 if success, 0 if failed
static int load_and_push_config(lua_State* L, const char* config_path) {
    char path[1152];

    // Use provided config_path if available, otherwise build default path
    if (config_path) {
        snprintf(path, sizeof(path), "%s", config_path);
    } else {
        if (build_config_path(path, sizeof(path)) != 0) {
            return 0;
        }
    }

    char* buffer = NULL;
    size_t len = 0;
    if (read_file_all(path, &buffer, &len) != 0) {
        return 0;
    }
    bson_error_t error;
    bson_t* doc = bson_new_from_json((const uint8_t*)buffer, (ssize_t)len, &error);
    free(buffer);
    if (!doc) {
        fprintf(stderr, "Failed to parse config: %s\n", error.message);
        return 0;
    }
    
    // Apply settings to global variables
    apply_log_queue_settings(doc);
    
    // Push to Lua
    push_bson_document(L, doc);
    bson_destroy(doc);
    return 1;
}

// Print log (Lua binding)
static int lua_log(lua_State* L) {
    const char* level_str = NULL;
    const char* msg = NULL;
    int nargs = lua_gettop(L);

    if (nargs >= 2) {
        level_str = luaL_checkstring(L, 1);
        msg = luaL_checkstring(L, 2);
    } else {
        msg = luaL_checkstring(L, 1);
        level_str = "info";
    }

    log_level_t level = logger_parse_level(level_str);
    logger_write(level, msg);

    return 0;
}

// Register all Lua bindings
void register_lua_bindings(lua_State* L, server_context_t* ctx) {
    // Initialize logger for server
    // Use process_name from context (passed from command-line)
    logger_init("server", ctx->process_name);
    
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
    
    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, lua_log, 1);
    lua_setfield(L, -2, "log");

    lua_pushcfunction(L, lua_is_closing_client);
    lua_setfield(L, -2, "is_closing");
    
    // Load and attach config
    const char* process_name = ctx->process_name ? ctx->process_name : "server";
    if (load_and_push_config(L, ctx->config_path)) {
        // Config loaded successfully, stack: [gear_table, config_table]
        lua_pushvalue(L, -1);  // Duplicate config table
        lua_setfield(L, -3, "config");  // gear.config = config_table
        
        // Extract common section
        lua_getfield(L, -1, "common");
        lua_setfield(L, -3, "common");  // gear.common = config.common
        
        // Extract process section
        lua_getfield(L, -1, process_name);
        lua_setfield(L, -3, "process");  // gear.process = config[process_name]
        
        lua_pop(L, 1);  // Pop config table
    } else {
        // Config load failed, set nil values
        lua_pushnil(L);
        lua_setfield(L, -2, "config");
        lua_pushnil(L);
        lua_setfield(L, -2, "common");
        lua_pushnil(L);
        lua_setfield(L, -2, "process");
    }
    
    // Set process name
    lua_pushstring(L, process_name);
    lua_setfield(L, -2, "process_name");
    
    // Set as global gear table
    lua_setglobal(L, "gear");
}
