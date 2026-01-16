#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
    char* data;
} write_req_t;

static void on_write_done(uv_write_t* req, int status) {
    write_req_t* wr = (write_req_t*)req;
    if (status < 0) {
        fprintf(stderr, "Write error: %s\n", uv_strerror(status));
    }
    free(wr->data);
    free(wr);
}

static int send_buffer(lua_State* L, client_t* client, const char* data, size_t len) {
    if (!client) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Invalid client");
        return 2;
    }
    if (uv_is_closing((uv_handle_t*)&client->handle)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Client is closing");
        return 2;
    }
    if (len > (size_t)UINT_MAX) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Data too large");
        return 2;
    }
    if (len == 0) {
        lua_pushboolean(L, 1);
        return 1;
    }

    write_req_t* wr = (write_req_t*)malloc(sizeof(write_req_t));
    if (!wr) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Out of memory");
        return 2;
    }
    wr->data = (char*)malloc(len);
    if (!wr->data) {
        free(wr);
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Out of memory");
        return 2;
    }
    memcpy(wr->data, data, len);
    wr->buf = uv_buf_init(wr->data, (unsigned int)len);

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
    
    if (!client) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "Invalid client");
        return 2;
    }
    
    if (!uv_is_closing((uv_handle_t*)&client->handle)) {
        uv_close((uv_handle_t*)&client->handle, NULL);
    }
    lua_pushboolean(L, 1);
    return 1;
}

// Check if client is closing (Lua binding)
static int lua_is_closing_client(lua_State* L) {
    client_t* client = (client_t*)lua_touserdata(L, 1);
    if (!client) {
        lua_pushboolean(L, 1);
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

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} log_level_t;

static const char* log_level_to_string(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO: return "INFO";
        case LOG_LEVEL_WARN: return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_FATAL: return "FATAL";
        default: return "INFO";
    }
}

static int equals_ignore_case(const char* a, const char* b) {
    if (!a || !b) {
        return 0;
    }
    while (*a != '\0' && *b != '\0') {
        unsigned char ca = (unsigned char)*a;
        unsigned char cb = (unsigned char)*b;
        if (tolower(ca) != tolower(cb)) {
            return 0;
        }
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static log_level_t parse_log_level(const char* level_str) {
    if (!level_str) {
        return LOG_LEVEL_INFO;
    }
    if (equals_ignore_case(level_str, "debug")) return LOG_LEVEL_DEBUG;
    if (equals_ignore_case(level_str, "info")) return LOG_LEVEL_INFO;
    if (equals_ignore_case(level_str, "warn")) return LOG_LEVEL_WARN;
    if (equals_ignore_case(level_str, "error")) return LOG_LEVEL_ERROR;
    if (equals_ignore_case(level_str, "fatal")) return LOG_LEVEL_FATAL;
    return LOG_LEVEL_INFO;
}

static int ensure_directory(const char* path) {
    uv_fs_t req;
    int r = uv_fs_mkdir(uv_default_loop(), &req, path, 0755, NULL);
    uv_fs_req_cleanup(&req);
    if (r == 0 || r == UV_EEXIST) {
        return 0;
    }
    return -1;
}

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

static int init_log_paths(char* server_file, size_t server_size) {
#ifdef _WIN32
    const char sep = '\\';
#else
    const char sep = '/';
#endif

    char project_root[1024];
    get_project_root(project_root, sizeof(project_root));

    char log_dir[1152];
    char server_dir[1184];
    snprintf(log_dir, sizeof(log_dir), "%s%c%s", project_root, sep, "log");
    snprintf(server_dir, sizeof(server_dir), "%s%c%s", log_dir, sep, "server");
    snprintf(server_file, server_size, "%s%c%s", server_dir, sep, "server.log");

    if (ensure_directory(log_dir) != 0 || ensure_directory(server_dir) != 0) {
        return -1;
    }
    return 0;
}

static void format_timestamp(char* buffer, size_t buffer_size) {
    uv_timespec64_t ts;
    if (uv_clock_gettime(UV_CLOCK_REALTIME, &ts) != 0) {
        snprintf(buffer, buffer_size, "0");
        return;
    }
    long long ms = (long long)ts.tv_sec * 1000LL + (long long)(ts.tv_nsec / 1000000LL);
    snprintf(buffer, buffer_size, "%lld", ms);
}

typedef struct log_node {
    struct log_node* next;
    size_t len;
    char data[1];
} log_node_t;

typedef struct log_queue {
    uv_mutex_t mutex;
    uv_cond_t cond;
    log_node_t* head;
    log_node_t* tail;
    int running;
    int initialized;
    FILE* fp;
    uv_thread_t thread;
    int thread_started;
} log_queue_t;

static log_queue_t g_log_queue = {0};
static int g_log_ready = 0;
static char g_server_log_file[1216];

static void log_queue_worker(void* arg) {
    log_queue_t* q = (log_queue_t*)arg;
    uv_mutex_lock(&q->mutex);
    while (q->running || q->head) {
        while (!q->head && q->running) {
            uv_cond_wait(&q->cond, &q->mutex);
        }
        while (q->head) {
            log_node_t* node = q->head;
            q->head = node->next;
            if (!q->head) {
                q->tail = NULL;
            }
            FILE* fp = q->fp;
            uv_mutex_unlock(&q->mutex);
            if (fp) {
                fwrite(node->data, 1, node->len, fp);
            }
            free(node);
            uv_mutex_lock(&q->mutex);
        }
        if (!q->head && q->fp) {
            fflush(q->fp);
        }
    }
    uv_mutex_unlock(&q->mutex);
}

static int log_queue_init(log_queue_t* q, const char* file_path) {
    if (q->initialized) {
        return q->fp ? 0 : -1;
    }
    if (uv_mutex_init(&q->mutex) != 0) {
        return -1;
    }
    if (uv_cond_init(&q->cond) != 0) {
        uv_mutex_destroy(&q->mutex);
        return -1;
    }
    q->fp = fopen(file_path, "a");
    if (!q->fp) {
        uv_cond_destroy(&q->cond);
        uv_mutex_destroy(&q->mutex);
        return -1;
    }
    q->head = NULL;
    q->tail = NULL;
    q->running = 1;
    q->initialized = 1;
    if (uv_thread_create(&q->thread, log_queue_worker, q) != 0) {
        fclose(q->fp);
        q->fp = NULL;
        q->running = 0;
        uv_cond_destroy(&q->cond);
        uv_mutex_destroy(&q->mutex);
        q->initialized = 0;
        return -1;
    }
    q->thread_started = 1;
    return 0;
}

static void log_queue_push(log_queue_t* q, const char* line, size_t len) {
    if (!q->initialized || !q->fp) {
        return;
    }
    log_node_t* node = (log_node_t*)malloc(sizeof(log_node_t) + len);
    if (!node) {
        return;
    }
    node->next = NULL;
    node->len = len;
    memcpy(node->data, line, len);

    uv_mutex_lock(&q->mutex);
    if (q->tail) {
        q->tail->next = node;
        q->tail = node;
    } else {
        q->head = node;
        q->tail = node;
    }
    uv_cond_signal(&q->cond);
    uv_mutex_unlock(&q->mutex);
}

static void log_queue_shutdown(log_queue_t* q) {
    if (!q->initialized) {
        return;
    }
    uv_mutex_lock(&q->mutex);
    q->running = 0;
    uv_cond_signal(&q->cond);
    uv_mutex_unlock(&q->mutex);

    if (q->thread_started) {
        uv_thread_join(&q->thread);
        q->thread_started = 0;
    }

    if (q->fp) {
        fflush(q->fp);
        fclose(q->fp);
        q->fp = NULL;
    }
    uv_cond_destroy(&q->cond);
    uv_mutex_destroy(&q->mutex);
    q->initialized = 0;
    q->head = NULL;
    q->tail = NULL;
}

void log_shutdown(void) {
    log_queue_shutdown(&g_log_queue);
    g_log_ready = 0;
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

    log_level_t level = parse_log_level(level_str);
    const char* level_label = log_level_to_string(level);
    char timestamp[32];
    char log_line[2048];

    format_timestamp(timestamp, sizeof(timestamp));
    snprintf(log_line, sizeof(log_line), "[%s] [%s] %s\n", timestamp, level_label, msg);

    FILE* out = (level >= LOG_LEVEL_ERROR) ? stderr : stdout;
    fputs(log_line, out);
    fflush(out);

    if (!g_log_ready) {
        g_log_ready = (init_log_paths(g_server_log_file, sizeof(g_server_log_file)) == 0) &&
                      (log_queue_init(&g_log_queue, g_server_log_file) == 0);
    }
    if (g_log_ready) {
        log_queue_push(&g_log_queue, log_line, strlen(log_line));
    }

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
    
    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, lua_log, 1);
    lua_setfield(L, -2, "log");

    lua_pushcfunction(L, lua_is_closing_client);
    lua_setfield(L, -2, "is_closing");
    
    // Set as global gear table
    lua_setglobal(L, "gear");
}
