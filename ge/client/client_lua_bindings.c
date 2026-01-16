#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

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

static int init_log_paths(char* client_file, size_t client_size) {
#ifdef _WIN32
    const char sep = '\\';
#else
    const char sep = '/';
#endif

    char project_root[1024];
    get_project_root(project_root, sizeof(project_root));

    char log_dir[1152];
    char client_dir[1184];
    snprintf(log_dir, sizeof(log_dir), "%s%c%s", project_root, sep, "log");
    snprintf(client_dir, sizeof(client_dir), "%s%c%s", log_dir, sep, "client");
    snprintf(client_file, client_size, "%s%c%s", client_dir, sep, "client.log");

    if (ensure_directory(log_dir) != 0 || ensure_directory(client_dir) != 0) {
        return -1;
    }
    return 0;
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
static char g_client_log_file[1216];

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
    char log_line[2048];
    snprintf(log_line, sizeof(log_line), "[Client] %s\n", msg);
    fputs(log_line, stdout);
    fflush(stdout);

    if (!g_log_ready) {
        g_log_ready = (init_log_paths(g_client_log_file, sizeof(g_client_log_file)) == 0) &&
                      (log_queue_init(&g_log_queue, g_client_log_file) == 0);
    }
    if (g_log_ready) {
        log_queue_push(&g_log_queue, log_line, strlen(log_line));
    }
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
