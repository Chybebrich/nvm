#pragma once

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "includes.h"
#include "protocol.h"

#define MAX_PROXY_CONNECTIONS 100
#define PROXY_BUFFER_SIZE 8192
#define PROXY_TIMEOUT 300

typedef struct proxy_connection {
    int client_fd;
    int remote_fd;
    struct sockaddr_in client_addr;
    struct sockaddr_in remote_addr;
    time_t start_time;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint8_t active;
    pthread_t thread;
} proxy_connection_t;

typedef struct proxy_server {
    int listen_fd;
    uint16_t port;
    uint8_t running;
    uint8_t auth_required;
    uint8_t max_connections;
    uint32_t bandwidth_limit;
    proxy_connection_t connections[MAX_PROXY_CONNECTIONS];
    pthread_mutex_t mutex;
    struct proxy_stats stats;
} proxy_server_t;

// Proxy initialization and control
BOOL proxy_init(void);
void proxy_start(uint16_t port, struct proxy_config *config);
void proxy_stop(void);
void proxy_get_stats(struct proxy_stats *stats);
uint8_t proxy_get_status(void);

// SOCKS5 protocol handling
int socks5_handle_client(int client_fd);
int socks5_handshake(int client_fd);
int socks5_auth(int client_fd);
int socks5_connect(int client_fd, struct sockaddr_in *remote_addr);
void socks5_relay(int client_fd, int remote_fd);

// Connection management
void proxy_handle_connection(proxy_connection_t *conn);
int proxy_add_connection(int client_fd, struct sockaddr_in *client_addr);
void proxy_remove_connection(int index);
void proxy_cleanup_connections(void);

// Utility functions
void proxy_forward_data(int src_fd, int dst_fd, uint64_t *byte_counter);
int proxy_set_nonblocking(int fd);
void proxy_update_stats(void);
