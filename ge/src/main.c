/*
 * ge - GearEngine Server
 * 
 * ge 是 GearEngine 的简称，表示一个基于 libuv、Lua 和 libbson 的服务器引擎。
 * ge = GearEngine (齿轮引擎)
 * gear 小而精，功能简单，但是多个gear可以组成一个复杂的系统
 * 
 * 功能：
 * - 使用 libuv 提供异步网络驱动
 * - 使用 Lua 提供脚本语言环境
 * - 使用 libbson 提供网络协议数据的解包和压包功能
 */

#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static server_context_t g_server;
static int g_running = 1;

// Signal handler
static void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down server...\n", sig);
    g_running = 0;
    server_stop(&g_server);
}

// Print usage information
static void print_usage(const char* program_name) {
    printf("Usage: %s <process_name> [script_path] [config_path]\n", program_name);
    printf("\n");
    printf("Arguments:\n");
    printf("  process_name  - Process name for logging (e.g., gate, game, chat)\n");
    printf("                  This determines the log directory: log/server/<process_name>/\n");
    printf("  script_path   - Path to Lua script (default: ../ge/example/example_server/chat_server.lua)\n");
    printf("  config_path   - Path to config file (default: conf/conf.json)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s gate\n", program_name);
    printf("  %s gate gate_server.lua\n", program_name);
    printf("  %s gate gate_server.lua ../conf/gate.json\n", program_name);
    printf("\n");
}

int main(int argc, char* argv[]) {
    const char* process_name = NULL;
    const char* script_path = NULL;
    const char* config_path = NULL;
    int port = 8080;

    // Default paths
    const char* default_script = "../ge/example/example_server/chat_server.lua";
    const char* default_config = "conf/conf.json";

    // Parse command line arguments
    // Usage: ge <process_name> [script_path] [config_path]
    if (argc < 2) {
        fprintf(stderr, "Error: Process name is required\n\n");
        print_usage(argv[0]);
        return 1;
    }

    // Argument 1: process_name (required)
    process_name = argv[1];

    // Argument 2: script_path (optional)
    if (argc > 2) {
        script_path = argv[2];
    } else {
        script_path = default_script;
    }

    // Argument 3: config_path (optional)
    if (argc > 3) {
        config_path = argv[3];
    } else {
        config_path = default_config;
    }

    // Register signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize server
    if (server_init(&g_server, port, script_path, process_name, config_path) != 0) {
        fprintf(stderr, "Server initialization failed\n");
        return 1;
    }

    // Start server
    printf("========================================\n");
    printf("ge (GearEngine) Server\n");
    printf("========================================\n");
    printf("Process:  %s\n", process_name);
    printf("Port:     %d\n", port);
    printf("Script:   %s\n", script_path);
    printf("Config:   %s\n", config_path);
    printf("Log:      log/server/%s/%s.log\n", process_name, process_name);
    printf("========================================\n");

    int r = server_start(&g_server);

    // Cleanup
    server_cleanup(&g_server);

    return r;
}
