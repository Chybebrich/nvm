#define _GNU_SOURCE

#ifdef DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <netdb.h>

#include "includes.h"
#include "proxy.h"
#include "util.h"

static int proxy_server_fd = -1;
static BOOL proxy_running = FALSE;
static struct proxy_client clients[PROXY_MAX_CLIENTS];

BOOL proxy_init(struct proxy_config *config)
{
    int i;
    
    // Initialize client array
    for (i = 0; i < PROXY_MAX_CLIENTS; i++)
    {
        clients[i].active = FALSE;
        clients[i].client_fd = -1;
        clients[i].target_fd = -1;
        clients[i].last_activity = 0;
    }
    
    // Set default port if not specified
    if (config->listen_port == 0)
        config->listen_port = PROXY_PORT_DEFAULT;
    
#ifdef DEBUG
    printf("[proxy] Initialized with port %d, auth: %s\n", 
           config->listen_port, config->require_auth ? "enabled" : "disabled");
#endif
    
    return TRUE;
}

void proxy_start(struct proxy_config *config)
{
    struct sockaddr_in server_addr;
    int i, max_fd, activity;
    fd_set read_fds;
    struct timeval timeout;
    
    // Create server socket
    if ((proxy_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
#ifdef DEBUG
        printf("[proxy] Failed to create server socket\n");
#endif
        return;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(proxy_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    NONBLOCK(proxy_server_fd);
    
    // Bind to port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(config->listen_port);
    
    if (bind(proxy_server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
#ifdef DEBUG
        printf("[proxy] Failed to bind to port %d\n", config->listen_port);
#endif
        close(proxy_server_fd);
        return;
    }
    
    // Listen for connections
    if (listen(proxy_server_fd, 10) == -1)
    {
#ifdef DEBUG
        printf("[proxy] Failed to listen on socket\n");
#endif
        close(proxy_server_fd);
        return;
    }
    
    proxy_running = TRUE;
    
#ifdef DEBUG
    printf("[proxy] Server started on port %d\n", config->listen_port);
#endif
    
    // Main proxy loop
    while (proxy_running)
    {
        FD_ZERO(&read_fds);
        FD_SET(proxy_server_fd, &read_fds);
        max_fd = proxy_server_fd;
        
        // Add active client sockets to set
        for (i = 0; i < PROXY_MAX_CLIENTS; i++)
        {
            if (clients[i].active)
            {
                FD_SET(clients[i].client_fd, &read_fds);
                if (clients[i].client_fd > max_fd)
                    max_fd = clients[i].client_fd;
                    
                if (clients[i].target_fd > 0)
                {
                    FD_SET(clients[i].target_fd, &read_fds);
                    if (clients[i].target_fd > max_fd)
                        max_fd = clients[i].target_fd;
                }
                
                // Check for timeout
                if (time(NULL) - clients[i].last_activity > PROXY_TIMEOUT)
                {
                    proxy_cleanup_client(&clients[i]);
                }
            }
        }
        
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0 && errno != EINTR)
            continue;
        
        // Accept new connection
        if (FD_ISSET(proxy_server_fd, &read_fds))
        {
            int client_fd;
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            
            client_fd = accept(proxy_server_fd, (struct sockaddr *)&client_addr, &addr_len);
            
            if (client_fd >= 0)
            {
                NONBLOCK(client_fd);
                
                // Find free slot
                for (i = 0; i < PROXY_MAX_CLIENTS; i++)
                {
                    if (!clients[i].active)
                    {
                        clients[i].client_fd = client_fd;
                        clients[i].target_fd = -1;
                        clients[i].active = TRUE;
                        clients[i].last_activity = time(NULL);
                        
                        // Handle SOCKS5 handshake in non-blocking way
                        proxy_handle_client(client_fd, config);
                        break;
                    }
                }
                
                // No free slots
                if (i == PROXY_MAX_CLIENTS)
                    close(client_fd);
            }
        }
        
        // Handle client data
        for (i = 0; i < PROXY_MAX_CLIENTS; i++)
        {
            if (!clients[i].active)
                continue;
                
            // Client -> Target relay
            if (FD_ISSET(clients[i].client_fd, &read_fds))
            {
                char buffer[PROXY_BUFFER_SIZE];
                int bytes_read = recv(clients[i].client_fd, buffer, sizeof(buffer), 0);
                
                if (bytes_read <= 0)
                {
                    proxy_cleanup_client(&clients[i]);
                    continue;
                }
                
                clients[i].last_activity = time(NULL);
                
                if (clients[i].target_fd > 0)
                {
                    send(clients[i].target_fd, buffer, bytes_read, MSG_NOSIGNAL);
                }
            }
            
            // Target -> Client relay
            if (clients[i].target_fd > 0 && FD_ISSET(clients[i].target_fd, &read_fds))
            {
                char buffer[PROXY_BUFFER_SIZE];
                int bytes_read = recv(clients[i].target_fd, buffer, sizeof(buffer), 0);
                
                if (bytes_read <= 0)
                {
                    proxy_cleanup_client(&clients[i]);
                    continue;
                }
                
                clients[i].last_activity = time(NULL);
                send(clients[i].client_fd, buffer, bytes_read, MSG_NOSIGNAL);
            }
        }
    }
    
    proxy_stop();
}

void proxy_stop(void)
{
    int i;
    
    proxy_running = FALSE;
    
    if (proxy_server_fd >= 0)
    {
        close(proxy_server_fd);
        proxy_server_fd = -1;
    }
    
    for (i = 0; i < PROXY_MAX_CLIENTS; i++)
    {
        if (clients[i].active)
            proxy_cleanup_client(&clients[i]);
    }
    
#ifdef DEBUG
    printf("[proxy] Server stopped\n");
#endif
}

void proxy_handle_client(int client_fd, struct proxy_config *config)
{
    if (proxy_socks5_handshake(client_fd, config) != 0)
    {
        close(client_fd);
        return;
    }
}

int proxy_socks5_handshake(int client_fd, struct proxy_config *config)
{
    unsigned char buffer[512];
    int bytes_read, i;
    
    // Read client greeting: VER | NMETHODS | METHODS
    bytes_read = recv(client_fd, buffer, 257, 0);
    
    if (bytes_read < 3 || buffer[0] != SOCKS5_VERSION)
    {
#ifdef DEBUG
        printf("[proxy] Invalid SOCKS5 handshake\n");
#endif
        return -1;
    }
    
    // Send method selection: VER | METHOD
    unsigned char response[2];
    response[0] = SOCKS5_VERSION;
    
    if (config->require_auth)
    {
        response[1] = SOCKS5_AUTH_PASSWORD;
        send(client_fd, response, 2, MSG_NOSIGNAL);
        
        // Perform authentication
        if (proxy_socks5_auth(client_fd, config) != 0)
            return -1;
    }
    else
    {
        response[1] = SOCKS5_AUTH_NONE;
        send(client_fd, response, 2, MSG_NOSIGNAL);
    }
    
    // Handle CONNECT request
    return proxy_socks5_connect(client_fd);
}

int proxy_socks5_auth(int client_fd, struct proxy_config *config)
{
    unsigned char buffer[512];
    int bytes_read;
    
    // Read auth request: VER | ULEN | UNAME | PLEN | PASSWD
    bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
    
    if (bytes_read < 5 || buffer[0] != 0x01)
        return -1;
    
    int ulen = buffer[1];
    char username[256] = {0};
    char password[256] = {0};
    
    memcpy(username, buffer + 2, ulen);
    int plen = buffer[2 + ulen];
    memcpy(password, buffer + 3 + ulen, plen);
    
    // Verify credentials
    unsigned char response[2] = {0x01, 0x00}; // Success
    
    if (config->auth_user && config->auth_pass)
    {
        if (strcmp(username, config->auth_user) != 0 || 
            strcmp(password, config->auth_pass) != 0)
        {
            response[1] = 0x01; // Failure
        }
    }
    
    send(client_fd, response, 2, MSG_NOSIGNAL);
    
    return response[1] == 0x00 ? 0 : -1;
}

int proxy_socks5_connect(int client_fd)
{
    unsigned char buffer[512];
    int bytes_read, i;
    uint32_t target_ip;
    uint16_t target_port;
    int target_fd;
    
    // Read CONNECT request: VER | CMD | RSV | ATYP | DST.ADDR | DST.PORT
    bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
    
    if (bytes_read < 10 || buffer[0] != SOCKS5_VERSION || buffer[1] != SOCKS5_CMD_CONNECT)
    {
        unsigned char response[10] = {SOCKS5_VERSION, SOCKS5_REP_GENERAL_FAILURE};
        send(client_fd, response, 10, MSG_NOSIGNAL);
        return -1;
    }
    
    // Parse target address
    if (buffer[3] == SOCKS5_ATYP_IPV4)
    {
        memcpy(&target_ip, buffer + 4, 4);
        memcpy(&target_port, buffer + 8, 2);
    }
    else if (buffer[3] == SOCKS5_ATYP_DOMAIN)
    {
        // Domain resolution needed
        int domain_len = buffer[4];
        char domain[256] = {0};
        
        // Check domain length to prevent buffer overflow
        if (domain_len > 255 || domain_len <= 0)
        {
            unsigned char response[10] = {SOCKS5_VERSION, SOCKS5_REP_GENERAL_FAILURE};
            send(client_fd, response, 10, MSG_NOSIGNAL);
            return -1;
        }
        
        memcpy(domain, buffer + 5, domain_len);
        memcpy(&target_port, buffer + 5 + domain_len, 2);
        
        // Simple DNS resolution (blocking - should be improved)
        struct hostent *he = gethostbyname(domain);
        if (he == NULL)
        {
            unsigned char response[10] = {SOCKS5_VERSION, SOCKS5_REP_HOST_UNREACHABLE};
            send(client_fd, response, 10, MSG_NOSIGNAL);
            return -1;
        }
        memcpy(&target_ip, he->h_addr_list[0], 4);
    }
    else
    {
        unsigned char response[10] = {SOCKS5_VERSION, SOCKS5_REP_GENERAL_FAILURE};
        send(client_fd, response, 10, MSG_NOSIGNAL);
        return -1;
    }
    
    target_port = ntohs(target_port);
    
    // Check access control
    if (!proxy_check_access(target_ip, target_port))
    {
        unsigned char response[10] = {SOCKS5_VERSION, SOCKS5_REP_CONNECTION_NOT_ALLOWED};
        send(client_fd, response, 10, MSG_NOSIGNAL);
        return -1;
    }
    
    // Connect to target
    target_fd = proxy_connect_target(target_ip, target_port);
    
    if (target_fd < 0)
    {
        unsigned char response[10] = {SOCKS5_VERSION, SOCKS5_REP_CONNECTION_REFUSED};
        send(client_fd, response, 10, MSG_NOSIGNAL);
        return -1;
    }
    
    // Store target_fd in client structure BEFORE sending success
    for (i = 0; i < PROXY_MAX_CLIENTS; i++)
    {
        if (clients[i].active && clients[i].client_fd == client_fd)
        {
            clients[i].target_fd = target_fd;
            NONBLOCK(target_fd);
            break;
        }
    }
    
    // Send success response AFTER storing target_fd
    unsigned char response[10] = {
        SOCKS5_VERSION, SOCKS5_REP_SUCCESS, 0x00, SOCKS5_ATYP_IPV4,
        0, 0, 0, 0,  // Bind address (0.0.0.0)
        0, 0         // Bind port (0)
    };
    send(client_fd, response, 10, MSG_NOSIGNAL);
    
    return 0;
}

int proxy_connect_target(uint32_t target_ip, uint16_t target_port)
{
    int fd;
    struct sockaddr_in target_addr;
    
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;
    
    target_addr.sin_family = AF_INET;
    target_addr.sin_addr.s_addr = target_ip;
    target_addr.sin_port = htons(target_port);
    
    if (connect(fd, (struct sockaddr *)&target_addr, sizeof(target_addr)) < 0)
    {
        close(fd);
        return -1;
    }
    
    return fd;
}

void proxy_cleanup_client(struct proxy_client *client)
{
    if (client->client_fd >= 0)
    {
        close(client->client_fd);
        client->client_fd = -1;
    }
    
    if (client->target_fd >= 0)
    {
        close(client->target_fd);
        client->target_fd = -1;
    }
    
    client->active = FALSE;
    client->last_activity = 0;
}

BOOL proxy_check_access(uint32_t target_ip, uint16_t target_port)
{
    unsigned char *ip_bytes = (unsigned char *)&target_ip;
    
    // Block localhost connections (127.0.0.0/8)
    if (ip_bytes[0] == 127)
        return FALSE;
    
    // Block private networks (optional - depends on use case)
    // 10.0.0.0/8
    if (ip_bytes[0] == 10)
        return FALSE;
    
    // 172.16.0.0/12
    if (ip_bytes[0] == 172 && (ip_bytes[1] >= 16 && ip_bytes[1] <= 31))
        return FALSE;
    
    // 192.168.0.0/16
    if (ip_bytes[0] == 192 && ip_bytes[1] == 168)
        return FALSE;
    
    // Allow all other connections
    return TRUE;
}
