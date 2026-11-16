#pragma once

#include "includes.h"

#define PROXY_PORT_DEFAULT 1080
#define PROXY_MAX_CLIENTS 256
#define PROXY_BUFFER_SIZE 8192
#define PROXY_TIMEOUT 300

// SOCKS5 protocol constants
#define SOCKS5_VERSION 0x05
#define SOCKS5_AUTH_NONE 0x00
#define SOCKS5_AUTH_PASSWORD 0x02
#define SOCKS5_CMD_CONNECT 0x01
#define SOCKS5_ATYP_IPV4 0x01
#define SOCKS5_ATYP_DOMAIN 0x03
#define SOCKS5_REP_SUCCESS 0x00
#define SOCKS5_REP_GENERAL_FAILURE 0x01
#define SOCKS5_REP_CONNECTION_NOT_ALLOWED 0x02
#define SOCKS5_REP_NETWORK_UNREACHABLE 0x03
#define SOCKS5_REP_HOST_UNREACHABLE 0x04
#define SOCKS5_REP_CONNECTION_REFUSED 0x05

struct proxy_client {
    int client_fd;
    int target_fd;
    time_t last_activity;
    BOOL active;
    char buffer[PROXY_BUFFER_SIZE];
};

struct proxy_config {
    port_t listen_port;
    char *auth_user;
    char *auth_pass;
    BOOL require_auth;
    BOOL use_encryption;
};

// Proxy server functions
BOOL proxy_init(struct proxy_config *config);
void proxy_start(struct proxy_config *config);
void proxy_stop(void);
void proxy_handle_client(int client_fd, struct proxy_config *config);

// SOCKS5 protocol handlers
int proxy_socks5_handshake(int client_fd, struct proxy_config *config);
int proxy_socks5_auth(int client_fd, struct proxy_config *config);
int proxy_socks5_connect(int client_fd);
void proxy_socks5_relay(int client_fd, int target_fd);

// Helper functions
int proxy_connect_target(uint32_t target_ip, uint16_t target_port);
void proxy_cleanup_client(struct proxy_client *client);
BOOL proxy_check_access(uint32_t target_ip, uint16_t target_port);
