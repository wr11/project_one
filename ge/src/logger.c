#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <uv.h>

// Log queue structures
typedef struct log_node {
    struct log_node* next;
    size_t len;
    char data[1];
} log_node_t;

typedef struct log_queue {
    uv_mutex_t mutex;
    uv_cond_t cond_not_empty;
    uv_cond_t cond_not_full;
    log_node_t* head;
    log_node_t* tail;
    int running;
    int initialized;
    FILE* fp;
    uv_thread_t thread;
    int thread_started;
    size_t count;
    size_t max_count;
    size_t warn_threshold;
    int warn_emitted;
} log_queue_t;

// Global state
static log_queue_t g_log_queue = {0};
static int g_log_ready = 0;
static char g_log_file_path[1216];
static size_t g_log_queue_max = 10000;
static size_t g_log_queue_warn = 8000;
static const char* g_process_type = "server";

// Helper: case-insensitive string comparison
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

// Helper: find last path separator
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

// Helper: ensure directory exists
static int ensure_directory(const char* path) {
    uv_fs_t req;
    int r = uv_fs_mkdir(uv_default_loop(), &req, path, 0755, NULL);
    uv_fs_req_cleanup(&req);
    if (r == 0 || r == UV_EEXIST) {
        return 0;
    }
    return -1;
}

// Helper: get project root directory
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

// Helper: initialize log file paths
static int init_log_paths(const char* process_type) {
#ifdef _WIN32
    const char sep = '\\';
#else
    const char sep = '/';
#endif

    char project_root[1024];
    get_project_root(project_root, sizeof(project_root));

    char log_dir[1152];
    char type_dir[1184];
    snprintf(log_dir, sizeof(log_dir), "%s%c%s", project_root, sep, "log");
    snprintf(type_dir, sizeof(type_dir), "%s%c%s", log_dir, sep, process_type);
    snprintf(g_log_file_path, sizeof(g_log_file_path), "%s%c%s.log", type_dir, sep, process_type);

    if (ensure_directory(log_dir) != 0 || ensure_directory(type_dir) != 0) {
        return -1;
    }
    return 0;
}

// Helper: format timestamp
static void format_timestamp(char* buffer, size_t buffer_size) {
    uv_timespec64_t ts;
    if (uv_clock_gettime(UV_CLOCK_REALTIME, &ts) != 0) {
        snprintf(buffer, buffer_size, "0");
        return;
    }
    long long ms = (long long)ts.tv_sec * 1000LL + (long long)(ts.tv_nsec / 1000000LL);
    snprintf(buffer, buffer_size, "%lld", ms);
}

// Log queue worker thread
static void log_queue_worker(void* arg) {
    log_queue_t* q = (log_queue_t*)arg;
    uv_mutex_lock(&q->mutex);
    while (q->running || q->head) {
        while (!q->head && q->running) {
            uv_cond_wait(&q->cond_not_empty, &q->mutex);
        }
        while (q->head) {
            log_node_t* node = q->head;
            q->head = node->next;
            if (!q->head) {
                q->tail = NULL;
            }
            q->count--;
            FILE* fp = q->fp;
            uv_cond_signal(&q->cond_not_full);
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
        if (q->count < q->warn_threshold) {
            q->warn_emitted = 0;
        }
    }
    uv_mutex_unlock(&q->mutex);
}

// Initialize log queue
static int log_queue_init(log_queue_t* q, const char* file_path) {
    if (q->initialized) {
        return q->fp ? 0 : -1;
    }
    if (uv_mutex_init(&q->mutex) != 0) {
        return -1;
    }
    if (uv_cond_init(&q->cond_not_empty) != 0) {
        uv_mutex_destroy(&q->mutex);
        return -1;
    }
    if (uv_cond_init(&q->cond_not_full) != 0) {
        uv_cond_destroy(&q->cond_not_empty);
        uv_mutex_destroy(&q->mutex);
        return -1;
    }
    q->fp = fopen(file_path, "a");
    if (!q->fp) {
        uv_cond_destroy(&q->cond_not_full);
        uv_cond_destroy(&q->cond_not_empty);
        uv_mutex_destroy(&q->mutex);
        return -1;
    }
    q->head = NULL;
    q->tail = NULL;
    q->running = 1;
    q->initialized = 1;
    q->count = 0;
    q->max_count = g_log_queue_max > 0 ? g_log_queue_max : 10000;
    q->warn_threshold = g_log_queue_warn > 0 ? g_log_queue_warn : 8000;
    if (q->warn_threshold > q->max_count) {
        q->warn_threshold = q->max_count;
    }
    q->warn_emitted = 0;
    if (uv_thread_create(&q->thread, log_queue_worker, q) != 0) {
        fclose(q->fp);
        q->fp = NULL;
        q->running = 0;
        uv_cond_destroy(&q->cond_not_full);
        uv_cond_destroy(&q->cond_not_empty);
        uv_mutex_destroy(&q->mutex);
        q->initialized = 0;
        return -1;
    }
    q->thread_started = 1;
    return 0;
}

// Push log to queue
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
    while (q->count >= q->max_count && q->running) {
        uv_cond_wait(&q->cond_not_full, &q->mutex);
    }
    if (!q->running) {
        uv_mutex_unlock(&q->mutex);
        free(node);
        return;
    }
    if (q->tail) {
        q->tail->next = node;
        q->tail = node;
    } else {
        q->head = node;
        q->tail = node;
    }
    q->count++;
    if (!q->warn_emitted && q->count >= q->warn_threshold) {
        fprintf(stderr, "[log] queue near limit: %zu/%zu\n", q->count, q->max_count);
        q->warn_emitted = 1;
    }
    uv_cond_signal(&q->cond_not_empty);
    uv_mutex_unlock(&q->mutex);
}

// Shutdown log queue
static void log_queue_shutdown(log_queue_t* q) {
    if (!q->initialized) {
        return;
    }
    uv_mutex_lock(&q->mutex);
    q->running = 0;
    uv_cond_broadcast(&q->cond_not_empty);
    uv_cond_broadcast(&q->cond_not_full);
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
    uv_cond_destroy(&q->cond_not_full);
    uv_cond_destroy(&q->cond_not_empty);
    uv_mutex_destroy(&q->mutex);
    q->initialized = 0;
    q->head = NULL;
    q->tail = NULL;
    q->count = 0;
}

// Public API implementations

const char* logger_level_to_string(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO: return "INFO";
        case LOG_LEVEL_WARN: return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_FATAL: return "FATAL";
        default: return "INFO";
    }
}

log_level_t logger_parse_level(const char* level_str) {
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

void logger_init(const char* process_type) {
    if (process_type) {
        g_process_type = process_type;
    }
}

void logger_set_queue_params(size_t max_count, size_t warn_threshold) {
    if (!g_log_ready) {
        g_log_queue_max = max_count > 0 ? max_count : 10000;
        g_log_queue_warn = warn_threshold > 0 ? warn_threshold : 8000;
        if (g_log_queue_warn > g_log_queue_max) {
            g_log_queue_warn = g_log_queue_max * 8 / 10;
        }
    }
}

void logger_write(log_level_t level, const char* message) {
    const char* level_label = logger_level_to_string(level);
    char timestamp[32];
    char log_line[2048];

    format_timestamp(timestamp, sizeof(timestamp));
    snprintf(log_line, sizeof(log_line), "[%s] [%s] %s\n", timestamp, level_label, message);

    // Output to console
    FILE* out = (level >= LOG_LEVEL_ERROR) ? stderr : stdout;
    fputs(log_line, out);
    fflush(out);

    // Initialize queue if needed
    if (!g_log_ready) {
        g_log_ready = (init_log_paths(g_process_type) == 0) &&
                      (log_queue_init(&g_log_queue, g_log_file_path) == 0);
    }
    
    // Write to file queue
    if (g_log_ready) {
        log_queue_push(&g_log_queue, log_line, strlen(log_line));
    }
}

void logger_shutdown(void) {
    log_queue_shutdown(&g_log_queue);
    g_log_ready = 0;
}
