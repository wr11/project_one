/*
 * Stress Test Client for GearEngine Chat Server
 * 
 * This tool creates multiple concurrent connections to test server performance:
 * - Concurrent connections
 * - Message throughput
 * - Response latency
 * - Connection stability
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uv.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <signal.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#endif

// Statistics
typedef struct {
    int total_connections;
    int active_connections;
    int successful_connections;
    int failed_connections;
    int messages_sent;
    int messages_received;
    int errors;
    double total_latency;
    int latency_count;
    time_t start_time;
} stats_t;

// Client context
typedef struct {
    uv_tcp_t socket;
    uv_connect_t connect_req;
    int client_id;
    int messages_to_send;
    int messages_sent;
    int messages_received;
    stats_t* stats;
    uv_timer_t timer;
    int connected;
    char buffer[1024];
} stress_client_t;

static stats_t g_stats;
static uv_loop_t* g_loop;
static int g_total_clients = 100;
static int g_messages_per_client = 10;
static int g_message_interval_ms = 100;
static const char* g_host = "localhost";
static int g_port = 8080;
static int g_running = 1;

// Allocate buffer
static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = (char*)malloc(suggested_size);
    buf->len = suggested_size;
}

// Read callback
static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    stress_client_t* client = (stress_client_t*)stream->data;
    
    if (nread < 0) {
        if (nread != UV_EOF) {
            g_stats.errors++;
        }
        free(buf->base);
        return;
    }
    
    if (nread > 0) {
        client->messages_received++;
        g_stats.messages_received++;
    }
    
    free(buf->base);
}

// Write callback
static void on_write(uv_write_t* req, int status) {
    if (status < 0) {
        g_stats.errors++;
    }
    free(req);
}

// Send message
static void send_message(stress_client_t* client) {
    if (!client->connected || client->messages_sent >= client->messages_to_send) {
        return;
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Client%d: Message %d\n", client->client_id, client->messages_sent + 1);
    
    uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
    uv_buf_t buf = uv_buf_init(msg, strlen(msg));
    
    if (uv_write(req, (uv_stream_t*)&client->socket, &buf, 1, on_write) == 0) {
        client->messages_sent++;
        g_stats.messages_sent++;
    } else {
        g_stats.errors++;
    }
}

// Forward declaration
static void on_client_close(uv_handle_t* handle);

// Timer callback - send messages periodically
static void on_timer(uv_timer_t* handle) {
    stress_client_t* client = (stress_client_t*)handle->data;
    
    if (client->connected && client->messages_sent < client->messages_to_send) {
        send_message(client);
    } else {
        uv_timer_stop(handle);
        if (client->messages_sent >= client->messages_to_send) {
            // All messages sent, close connection
            uv_close((uv_handle_t*)&client->socket, on_client_close);
        }
    }
}

// Connect callback
static void on_connect(uv_connect_t* req, int status) {
    stress_client_t* client = (stress_client_t*)req->data;
    
    if (status < 0) {
        g_stats.failed_connections++;
        g_stats.active_connections--;
        free(client);
        return;
    }
    
    client->connected = 1;
    g_stats.successful_connections++;
    
    // Start reading
    uv_read_start((uv_stream_t*)&client->socket, alloc_buffer, on_read);
    
    // Start timer to send messages
    uv_timer_init(g_loop, &client->timer);
    client->timer.data = client;
    uv_timer_start(&client->timer, on_timer, 0, g_message_interval_ms);
}

// Create a stress client
static void create_client(int client_id) {
    stress_client_t* client = (stress_client_t*)malloc(sizeof(stress_client_t));
    memset(client, 0, sizeof(stress_client_t));
    
    client->client_id = client_id;
    client->messages_to_send = g_messages_per_client;
    client->stats = &g_stats;
    client->connected = 0;
    
    uv_tcp_init(g_loop, &client->socket);
    client->socket.data = client;
    client->connect_req.data = client;
    
    struct sockaddr_in addr;
    uv_ip4_addr(g_host, g_port, &addr);
    
    g_stats.active_connections++;
    uv_tcp_connect(&client->connect_req, &client->socket, 
                   (const struct sockaddr*)&addr, on_connect);
}

// Close callback
static void on_client_close(uv_handle_t* handle) {
    stress_client_t* client = (stress_client_t*)handle->data;
    if (client) {
        uv_timer_stop(&client->timer);
        uv_close((uv_handle_t*)&client->timer, NULL);
        g_stats.active_connections--;
        free(client);
    }
}

// Print statistics
static void print_stats() {
    time_t now = time(NULL);
    double elapsed = difftime(now, g_stats.start_time);
    
    printf("\n=== Stress Test Statistics ===\n");
    printf("Total connections attempted: %d\n", g_stats.total_connections);
    printf("Successful connections: %d\n", g_stats.successful_connections);
    printf("Failed connections: %d\n", g_stats.failed_connections);
    printf("Active connections: %d\n", g_stats.active_connections);
    printf("Messages sent: %d\n", g_stats.messages_sent);
    printf("Messages received: %d\n", g_stats.messages_received);
    printf("Errors: %d\n", g_stats.errors);
    printf("Elapsed time: %.2f seconds\n", elapsed);
    
    if (elapsed > 0) {
        printf("Messages per second: %.2f\n", g_stats.messages_sent / elapsed);
        printf("Connections per second: %.2f\n", g_stats.successful_connections / elapsed);
    }
    
    if (g_stats.latency_count > 0) {
        printf("Average latency: %.2f ms\n", g_stats.total_latency / g_stats.latency_count);
    }
    
    printf("==============================\n");
}

// Periodic stats printer
static void print_stats_timer(uv_timer_t* handle) {
    print_stats();
}

// Cleanup
static void cleanup() {
    uv_stop(g_loop);
    print_stats();
}

// Signal handler
#ifdef _WIN32
static BOOL WINAPI signal_handler(DWORD dwCtrlType) {
    printf("\nReceived signal, stopping stress test...\n");
    g_running = 0;
    cleanup();
    return TRUE;
}
#else
static void signal_handler(int sig) {
    printf("\nReceived signal %d, stopping stress test...\n", sig);
    g_running = 0;
    cleanup();
}
#endif

int main(int argc, char* argv[]) {
    // Parse arguments
    if (argc > 1) {
        g_total_clients = atoi(argv[1]);
    }
    if (argc > 2) {
        g_messages_per_client = atoi(argv[2]);
    }
    if (argc > 3) {
        g_message_interval_ms = atoi(argv[3]);
    }
    if (argc > 4) {
        g_host = argv[4];
    }
    if (argc > 5) {
        g_port = atoi(argv[5]);
    }
    
    printf("=== GearEngine Stress Test ===\n");
    printf("Host: %s\n", g_host);
    printf("Port: %d\n", g_port);
    printf("Total clients: %d\n", g_total_clients);
    printf("Messages per client: %d\n", g_messages_per_client);
    printf("Message interval: %d ms\n", g_message_interval_ms);
    printf("==============================\n\n");
    
    // Initialize statistics
    memset(&g_stats, 0, sizeof(stats_t));
    g_stats.start_time = time(NULL);
    
    // Initialize event loop
    g_loop = uv_default_loop();
    
    // Create clients
    printf("Creating %d clients...\n", g_total_clients);
    for (int i = 0; i < g_total_clients; i++) {
        g_stats.total_connections++;
        create_client(i + 1);
        
        // Stagger connections slightly
        if (i % 10 == 0) {
            uv_run(g_loop, UV_RUN_NOWAIT);
        }
    }
    
    // Start periodic stats printing
    uv_timer_t stats_timer;
    uv_timer_init(g_loop, &stats_timer);
    uv_timer_start(&stats_timer, print_stats_timer, 5000, 5000); // Print every 5 seconds
    
    // Run event loop
    // Register signal handlers
#ifdef _WIN32
    SetConsoleCtrlHandler(signal_handler, TRUE);
#else
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
#endif
    
    printf("Running stress test... (Press Ctrl+C to stop)\n");
    uv_run(g_loop, UV_RUN_DEFAULT);
    
    // Final cleanup
    cleanup();
    
    return 0;
}
