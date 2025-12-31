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

int main(int argc, char* argv[]) {
    int port = 8080;
    const char* script_path = NULL;
    
    // Default script path: ../server/chat_server.lua (relative to executable)
    // This assumes executable is in server/ directory
    const char* default_script = "chat_server.lua";
    
    // Parse command line arguments
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    if (argc > 2) {
        script_path = argv[2];
    } else {
        // Use default script if not specified
        script_path = default_script;
    }
    
    // Register signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize server
    if (server_init(&g_server, port, script_path) != 0) {
        fprintf(stderr, "Server initialization failed\n");
        return 1;
    }
    
    // Start server
    printf("ge (GearEngine) server starting...\n");
    printf("Port: %d\n", port);
    if (script_path) {
        printf("Script: %s\n", script_path);
    }
    
    int r = server_start(&g_server);
    
    // Cleanup
    server_cleanup(&g_server);
    
    return r;
}
