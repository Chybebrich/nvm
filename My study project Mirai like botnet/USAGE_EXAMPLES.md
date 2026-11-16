# Примеры использования резидентских прокси

## Python Examples

### 1. Базовое использование с requests

```python
import requests

# Прокси конфигурация
proxies = {
    'http': 'socks5://bot_ip:1080',
    'https': 'socks5://bot_ip:1080'
}

# Простой GET запрос
response = requests.get('https://api.ipify.org?format=json', proxies=proxies)
print(f"Мой IP через прокси: {response.json()['ip']}")

# POST запрос
data = {'key': 'value'}
response = requests.post('https://httpbin.org/post', json=data, proxies=proxies)
print(response.json())
```

### 2. С аутентификацией

```python
import requests

# Если прокси требует аутентификацию
proxies = {
    'http': 'socks5://username:password@bot_ip:1080',
    'https': 'socks5://username:password@bot_ip:1080'
}

response = requests.get('https://api.ipify.org', proxies=proxies)
print(response.text)
```

### 3. Ротация через несколько ботов

```python
import requests
import random

# Список доступных прокси (ботов)
proxy_list = [
    'socks5://bot1_ip:1080',
    'socks5://bot2_ip:1080',
    'socks5://bot3_ip:1080',
    'socks5://bot4_ip:1080',
]

def get_random_proxy():
    proxy = random.choice(proxy_list)
    return {
        'http': proxy,
        'https': proxy
    }

# Использование
for i in range(10):
    proxies = get_random_proxy()
    response = requests.get('https://api.ipify.org', proxies=proxies)
    print(f"Запрос {i+1}: IP = {response.text}")
```

### 4. Класс ProxyManager

```python
import requests
import random
from typing import List, Dict
import time

class ProxyManager:
    def __init__(self, proxy_list: List[str]):
        self.proxy_list = proxy_list
        self.current_index = 0
        
    def get_next_proxy(self) -> Dict[str, str]:
        """Получить следующий прокси (round-robin)"""
        proxy = self.proxy_list[self.current_index]
        self.current_index = (self.current_index + 1) % len(self.proxy_list)
        return {
            'http': f'socks5://{proxy}',
            'https': f'socks5://{proxy}'
        }
    
    def get_random_proxy(self) -> Dict[str, str]:
        """Получить случайный прокси"""
        proxy = random.choice(self.proxy_list)
        return {
            'http': f'socks5://{proxy}',
            'https': f'socks5://{proxy}'
        }
    
    def test_proxy(self, proxy: str, timeout: int = 5) -> bool:
        """Проверить работоспособность прокси"""
        try:
            proxies = {
                'http': f'socks5://{proxy}',
                'https': f'socks5://{proxy}'
            }
            response = requests.get(
                'https://api.ipify.org',
                proxies=proxies,
                timeout=timeout
            )
            return response.status_code == 200
        except:
            return False
    
    def get_working_proxies(self) -> List[str]:
        """Получить список работающих прокси"""
        working = []
        for proxy in self.proxy_list:
            print(f"Тестирую {proxy}...", end=' ')
            if self.test_proxy(proxy):
                working.append(proxy)
                print("✓")
            else:
                print("✗")
        return working

# Использование
proxies = [
    '192.168.1.100:1080',
    '192.168.1.101:1080',
    '192.168.1.102:1080',
]

manager = ProxyManager(proxies)

# Получить работающие прокси
working = manager.get_working_proxies()
print(f"\nРаботает {len(working)} из {len(proxies)} прокси")

# Сделать запросы через разные прокси
for i in range(5):
    proxy = manager.get_next_proxy()
    response = requests.get('https://api.ipify.org', proxies=proxy)
    print(f"Запрос {i+1}: IP = {response.text}")
```

## cURL Examples

### 1. Простой запрос

```bash
# GET запрос
curl -x socks5://bot_ip:1080 https://api.ipify.org

# С аутентификацией
curl -x socks5://username:password@bot_ip:1080 https://api.ipify.org

# POST запрос
curl -x socks5://bot_ip:1080 \
  -X POST \
  -H "Content-Type: application/json" \
  -d '{"key":"value"}' \
  https://httpbin.org/post
```

### 2. Скачать файл через прокси

```bash
curl -x socks5://bot_ip:1080 \
  -o output.txt \
  https://example.com/file.txt
```

### 3. Verbose режим (отладка)

```bash
curl -v -x socks5://bot_ip:1080 https://api.ipify.org
```

## Proxychains Examples

### 1. Настройка

```bash
# Установка
sudo apt install proxychains4

# Конфигурация /etc/proxychains4.conf
sudo nano /etc/proxychains4.conf
```

Добавить в конец файла:
```
# Residential proxies
socks5 bot1_ip 1080
socks5 bot2_ip 1080
socks5 bot3_ip 1080
```

### 2. Использование

```bash
# Через один прокси (strict_chain)
proxychains4 curl https://api.ipify.org

# Случайный прокси (random_chain)
proxychains4 firefox

# Любая команда
proxychains4 python script.py
proxychains4 nmap -sT target.com
proxychains4 ssh user@host
```

## Node.js Examples

### 1. С модулем socks-proxy-agent

```javascript
const fetch = require('node-fetch');
const { SocksProxyAgent } = require('socks-proxy-agent');

// Создать прокси агент
const agent = new SocksProxyAgent('socks5://bot_ip:1080');

// GET запрос
fetch('https://api.ipify.org?format=json', { agent })
  .then(res => res.json())
  .then(json => console.log('IP:', json.ip))
  .catch(err => console.error(err));

// С аутентификацией
const authAgent = new SocksProxyAgent('socks5://user:pass@bot_ip:1080');

fetch('https://api.ipify.org', { agent: authAgent })
  .then(res => res.text())
  .then(ip => console.log('IP:', ip));
```

### 2. С Axios

```javascript
const axios = require('axios');
const { SocksProxyAgent } = require('socks-proxy-agent');

const agent = new SocksProxyAgent('socks5://bot_ip:1080');

axios.get('https://api.ipify.org?format=json', {
  httpAgent: agent,
  httpsAgent: agent
})
.then(response => {
  console.log('IP:', response.data.ip);
})
.catch(error => {
  console.error(error);
});
```

## Go Examples

```go
package main

import (
    "context"
    "fmt"
    "io/ioutil"
    "net/http"
    "net/url"
    
    "golang.org/x/net/proxy"
)

func main() {
    // Создать SOCKS5 dialer
    dialer, err := proxy.SOCKS5("tcp", "bot_ip:1080", nil, proxy.Direct)
    if err != nil {
        panic(err)
    }
    
    // Создать HTTP transport с прокси
    transport := &http.Transport{
        DialContext: func(ctx context.Context, network, addr string) (net.Conn, error) {
            return dialer.Dial(network, addr)
        },
    }
    
    // Создать HTTP client
    client := &http.Client{
        Transport: transport,
    }
    
    // Сделать запрос
    resp, err := client.Get("https://api.ipify.org")
    if err != nil {
        panic(err)
    }
    defer resp.Body.Close()
    
    body, _ := ioutil.ReadAll(resp.Body)
    fmt.Println("IP:", string(body))
}
```

## PHP Examples

```php
<?php

// С cURL
$ch = curl_init();
curl_setopt($ch, CURLOPT_URL, "https://api.ipify.org");
curl_setopt($ch, CURLOPT_PROXY, "socks5://bot_ip:1080");
curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);

$response = curl_exec($ch);
if (curl_errno($ch)) {
    echo 'Error: ' . curl_error($ch);
} else {
    echo 'IP: ' . $response;
}
curl_close($ch);

// С аутентификацией
curl_setopt($ch, CURLOPT_PROXYUSERPWD, "username:password");

?>
```

## Selenium/Browser Automation

### Python + Selenium

```python
from selenium import webdriver
from selenium.webdriver.chrome.options import Options

# Chrome с SOCKS5 прокси
options = Options()
options.add_argument('--proxy-server=socks5://bot_ip:1080')

driver = webdriver.Chrome(options=options)
driver.get('https://api.ipify.org')
print(driver.page_source)
driver.quit()
```

### Puppeteer (Node.js)

```javascript
const puppeteer = require('puppeteer');

(async () => {
  const browser = await puppeteer.launch({
    args: [
      '--proxy-server=socks5://bot_ip:1080'
    ]
  });
  
  const page = await browser.newPage();
  await page.goto('https://api.ipify.org');
  
  const content = await page.content();
  console.log(content);
  
  await browser.close();
})();
```

## Проверка работоспособности

### Bash скрипт для тестирования всех прокси

```bash
#!/bin/bash

# Список прокси
proxies=(
    "192.168.1.100:1080"
    "192.168.1.101:1080"
    "192.168.1.102:1080"
)

echo "Тестирование прокси..."

for proxy in "${proxies[@]}"; do
    echo -n "Проверяю $proxy... "
    
    # Тест с таймаутом 5 секунд
    ip=$(curl -s --connect-timeout 5 \
         -x "socks5://$proxy" \
         https://api.ipify.org 2>/dev/null)
    
    if [ $? -eq 0 ] && [ ! -z "$ip" ]; then
        echo "✓ OK (IP: $ip)"
    else
        echo "✗ FAILED"
    fi
done
```

### Python скрипт мониторинга

```python
import requests
import time
from datetime import datetime

PROXIES = [
    '192.168.1.100:1080',
    '192.168.1.101:1080',
    '192.168.1.102:1080',
]

def check_proxy(proxy, timeout=5):
    try:
        proxies = {
            'http': f'socks5://{proxy}',
            'https': f'socks5://{proxy}'
        }
        start = time.time()
        response = requests.get(
            'https://api.ipify.org',
            proxies=proxies,
            timeout=timeout
        )
        elapsed = time.time() - start
        
        if response.status_code == 200:
            return {
                'status': 'OK',
                'ip': response.text,
                'latency': f'{elapsed:.2f}s'
            }
    except Exception as e:
        return {'status': 'FAIL', 'error': str(e)}

def monitor_proxies(interval=60):
    """Мониторить прокси каждые N секунд"""
    while True:
        print(f"\n[{datetime.now().strftime('%H:%M:%S')}] Проверка прокси...")
        print("-" * 60)
        
        for proxy in PROXIES:
            result = check_proxy(proxy)
            status = "✓" if result['status'] == 'OK' else "✗"
            
            if result['status'] == 'OK':
                print(f"{status} {proxy:20} - {result['ip']:15} ({result['latency']})")
            else:
                print(f"{status} {proxy:20} - {result.get('error', 'Unknown error')}")
        
        time.sleep(interval)

if __name__ == '__main__':
    monitor_proxies(interval=60)  # Проверять каждую минуту
```

## Tips & Tricks

### 1. Балансировка нагрузки

```python
from collections import deque
import requests

class LoadBalancer:
    def __init__(self, proxies):
        self.proxies = deque(proxies)
    
    def get_proxy(self):
        proxy = self.proxies[0]
        self.proxies.rotate(-1)  # Переместить в конец
        return {
            'http': f'socks5://{proxy}',
            'https': f'socks5://{proxy}'
        }

lb = LoadBalancer(['bot1:1080', 'bot2:1080', 'bot3:1080'])

for i in range(10):
    response = requests.get('https://httpbin.org/ip', proxies=lb.get_proxy())
    print(response.json())
```

### 2. Retry логика

```python
import requests
from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry

def create_session_with_retries():
    session = requests.Session()
    
    retry = Retry(
        total=3,
        backoff_factor=1,
        status_forcelist=[500, 502, 503, 504]
    )
    
    adapter = HTTPAdapter(max_retries=retry)
    session.mount('http://', adapter)
    session.mount('https://', adapter)
    
    return session

session = create_session_with_retries()
session.proxies = {
    'http': 'socks5://bot_ip:1080',
    'https': 'socks5://bot_ip:1080'
}

response = session.get('https://api.ipify.org')
```

### 3. Асинхронные запросы

```python
import asyncio
import aiohttp
from aiohttp_socks import ProxyConnector

async def fetch(url, proxy):
    connector = ProxyConnector.from_url(f'socks5://{proxy}')
    async with aiohttp.ClientSession(connector=connector) as session:
        async with session.get(url) as response:
            return await response.text()

async def main():
    proxies = ['bot1:1080', 'bot2:1080', 'bot3:1080']
    tasks = [fetch('https://api.ipify.org', proxy) for proxy in proxies]
    results = await asyncio.gather(*tasks)
    
    for proxy, ip in zip(proxies, results):
        print(f"{proxy} -> {ip}")

asyncio.run(main())
```
