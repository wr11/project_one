#ifndef GEARENGINE_SERVER_H
#define GEARENGINE_SERVER_H

#include <uv.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <bson/bson.h>

// Constants
#define CLIENT_BUFFER_SIZE 4096
#define CLIENT_MAGIC 0x47454152  // 'GEAR' in hex
#define LISTEN_BACKLOG 128
#define INITIAL_CLIENT_CAPACITY 16
#define MESSAGE_HEADER_SIZE 4    // 4-byte length prefix

// Client connection structure
typedef struct {
    uv_tcp_t handle;
    char buffer[CLIENT_BUFFER_SIZE];
    int buffer_used;             // Bytes used in receive buffer
    int expected_length;         // Expected message length (from header)
    struct server_context* server;
    int array_index;             // Index in server's client array (for O(1) removal)
    uint32_t magic;              // Magic number for validity check
    int is_valid;                // Validity flag (0 = closed/invalid, 1 = valid)
    uv_timer_t idle_timer;       // Idle timeout timer
    uint64_t last_activity;      // Last activity timestamp (ms)
} client_t;

// Server context structure
typedef struct server_context {
    uv_loop_t* loop;
    uv_tcp_t server_handle;
    lua_State* L;
    int port;
    char* script_path;
    char* process_name;      // Process name for logging (e.g., "gate", "game", "chat")
    char* config_path;       // Path to config file (e.g., "server/conf/conf.json")
    client_t** clients;
    int client_count;
    int client_capacity;
} server_context_t;

// Server functions
int server_init(server_context_t* ctx, int port, const char* script_path,
                const char* process_name, const char* config_path);
int server_start(server_context_t* ctx);
void server_stop(server_context_t* ctx);
void server_cleanup(server_context_t* ctx);

// Client callback (exposed for Lua bindings)
void on_client_close(uv_handle_t* handle);

// Lua binding functions
void register_lua_bindings(lua_State* L, server_context_t* ctx);

// BSON handling functions
bson_t* bson_from_buffer(const char* buffer, int len);
int bson_to_buffer(bson_t* bson, char* buffer, int buffer_size);

#endif // GEARENGINE_SERVER_H
