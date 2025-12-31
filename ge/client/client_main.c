#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

static client_context_t g_client;
static int g_running = 1;

// Signal handler
static void signal_handler(int sig) {
    printf("\nReceived signal %d, disconnecting...\n", sig);
    g_running = 0;
    client_disconnect(&g_client);
}

int main(int argc, char* argv[]) {
    const char* host = "localhost";
    int port = 8080;
    const char* script_path = NULL;
    
    // Default script path: ../client/chat_client.lua (relative to executable)
    // This assumes executable is in client/ directory
    const char* default_script = "chat_client.lua";
    
    // Parse command line arguments
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = atoi(argv[2]);
    }
    if (argc > 3) {
        script_path = argv[3];
    } else {
        // Use default script if not specified
        script_path = default_script;
    }
    
    // Register signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize client
    if (client_init(&g_client, host, port, script_path) != 0) {
        fprintf(stderr, "Client initialization failed\n");
        return 1;
    }
    
    printf("Connecting to %s:%d...\n", host, port);
    
    // Connect to server
    if (client_connect(&g_client) != 0) {
        fprintf(stderr, "Failed to connect\n");
        client_cleanup(&g_client);
        return 1;
    }
    
    // Run event loop (blocking)
    // Input is handled by TTY callback in client.c
    uv_run(g_client.loop, UV_RUN_DEFAULT);
    
    // Cleanup
    client_cleanup(&g_client);
    
    return 0;
}
