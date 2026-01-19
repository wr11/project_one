#ifndef GEARENGINE_LOGGER_H
#define GEARENGINE_LOGGER_H

#include <stddef.h>

// Log levels
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} log_level_t;

// Initialize logger with process type and optional process name
// process_type: "server" or "client"
// process_name: optional process name for server (e.g., "gate", "game", "chat")
//               if NULL, uses process_type as name
// Example:
//   logger_init("server", "gate")  -> log/server/gate/gate.log
//   logger_init("server", NULL)    -> log/server/server/server.log
//   logger_init("client", NULL)    -> log/client/client.log
void logger_init(const char* process_type, const char* process_name);

// Set queue parameters (must be called before first log)
void logger_set_queue_params(size_t max_count, size_t warn_threshold);

// Write a log message
void logger_write(log_level_t level, const char* message);

// Shutdown logger and flush all pending logs
void logger_shutdown(void);

// Convert log level to string
const char* logger_level_to_string(log_level_t level);

// Parse log level from string
log_level_t logger_parse_level(const char* level_str);

#endif // GEARENGINE_LOGGER_H
