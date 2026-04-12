# Check-list-bot

Telegram-бот для управления задачами и чек-листами, написанный на C++ с использованием современных библиотек.

## Описание

Check-list-bot — это бот для Telegram, который помогает пользователям создавать, отслеживать и завершать задачи в формате чек-листов. Бот использует SQLite для хранения данных, предоставляет интерактивные клавиатуры и логирует действия с помощью spdlog.

Проект построен на CMake, управляет зависимостями через Conan и поддерживает сборку в Docker-контейнере.

## Особенности

- Создание, редактирование, удаление задач
- Интерактивные inline- и reply-клавиатуры
- Логирование с разными уровнями (info, warning, error)
- Хранение данных в SQLite с использованием SQLiteCpp
- Поддержка многопользовательского режима
- Конфигурация через переменные окружения
- Graceful shutdown по `SIGINT` и `SIGTERM`
- Сборка с помощью CMake и Conan

## Технологии

- **C++23** – современный стандарт C++
- **tgbot-cpp** – библиотека для Telegram Bot API
- **SQLiteCpp** – обёртка над SQLite3
- **spdlog** – быстрая библиотека логирования
- **magic_enum** – работа с перечислениями в compile-time
- **CMake** – система сборки
- **Conan** – менеджер зависимостей C++
- **Docker** – контейнеризация

## Установка и запуск

### Предварительные требования

- Компилятор C++ с поддержкой C++23 (g++ >= 13, clang >= 16)
- CMake >= 3.31
- Conan 2
- SQLite3 (системная библиотека)
- Python 3.12 (для управления зависимостями через Poetry, опционально)

### Шаги сборки

1. Клонируйте репозиторий:

   ```bash
   git clone https://github.com/Joraslav/check-list-bot.git
   cd check-list-bot
   ```

2. Установите зависимости Conan (для Release или Debug):

   ```bash
   # Release
   conan install . --build=missing -pr:h profiles/Release -pr:b profiles/Release
   # Debug
   conan install . --build=missing -pr:h profiles/Debug -pr:b profiles/Debug
   ```

3. Соберите проект:

   ```bash
   cmake --preset conan-release   # или conan-debug для Debug
   cmake --build --preset conan-release
   ```

4. Запустите бота:

   ```bash
   TELEGRAM_BOT_TOKEN="your-telegram-bot-token" ./check-list-bot
   ```

### Использование Docker

Соберите runtime-образ:

```bash
docker build -f deploy/docker/Dockerfile --target runtime -t check-list-bot:latest .
```

Запустите контейнер напрямую:

```bash
docker run -d \
   --name check-list-bot \
   --restart unless-stopped \
   -e TELEGRAM_BOT_TOKEN="your-telegram-bot-token" \
   -e TELEGRAM_BOT_DB_PATH="/app/data/tasks.db" \
   -e TELEGRAM_BOT_LOG_DIR="/app/logs" \
   -e TELEGRAM_BOT_LONG_POLL_TIMEOUT_SEC="1" \
   -v $(pwd)/data:/app/data \
   -v $(pwd)/logs:/app/logs \
   check-list-bot:latest
```

Для запуска тестов через multi-stage target:

```bash
docker build -f deploy/docker/Dockerfile --target test -t check-list-bot:test .
docker run --rm -v $(pwd)/reports:/app/reports check-list-bot:test
```

### Docker Compose на удалённом сервере

1. Скопируйте [compose.yml](deploy/docker/compose.yml) и создайте `deploy/docker/.env` на основе [.env.example](deploy/docker/.env.example).
2. Укажите `TELEGRAM_BOT_TOKEN` и при необходимости свой тег в `CHECK_LIST_BOT_IMAGE`.
3. Запустите сервис одной командой:

```bash
./scripts/deploy_docker.sh
```

Эквивалентные команды вручную:

```bash
cp deploy/docker/.env.example deploy/docker/.env
docker compose -f deploy/docker/compose.yml --env-file deploy/docker/.env pull
docker compose -f deploy/docker/compose.yml --env-file deploy/docker/.env up -d
```

Полезные команды:

```bash
./scripts/deploy_docker.sh logs
./scripts/deploy_docker.sh status
./scripts/deploy_docker.sh restart
```

По умолчанию Compose использует именованные volume для базы и логов, поэтому данные сохраняются между перезапусками контейнера.

## Структура проекта

```plaintext
check-list-bot/
├── src/
│   ├── bot/           – логика бота, обработчики команд
│   ├── database/      – работа с БД, модели задач
│   ├── log/           – модуль логирования
│   └── utils/         – вспомогательные утилиты
├── cmake/             – дополнительные CMake-скрипты
├── profiles/          – профили Conan (Debug/Release)
├── scripts/           – вспомогательные скрипты
├── deploy/docker/     – Dockerfile, compose и entrypoint/healthcheck для деплоя
├── .github/workflows/ – CI/CD (форматирование, сборка)
├── CMakeLists.txt     – корневой CMake
├── conanfile.txt      – зависимости Conan
├── pyproject.toml     – конфигурация Poetry (для dev-инструментов)
└── main.cpp           – точка входа
```

## Разработка

### Форматирование кода

Проект использует clang-format. Для автоформатирования используйте:

```bash
./scripts/fix_style.sh
```

Чтобы только вывести список файлов для форматирования:

```bash
./scripts/fix_style.sh --list
```

### Тестирование

В проекте есть unit-тесты на Google Test для клавиатур, statement manager, scoped statement,
transaction guard и `TaskDB`.

Запуск через CTest:

```bash
ctest --test-dir build --output-on-failure
```

Для Docker-сценария можно использовать stage `test` из [Dockerfile](deploy/docker/Dockerfile).

### Статический анализ

Для проверки clang-tidy используйте:

```bash
./scripts/check_tidy.sh
```

Полезные варианты:

```bash
./scripts/check_tidy.sh --list
./scripts/check_tidy.sh --fix
./scripts/check_tidy.sh --build-dir build
```

### Логирование

Логи пишутся одновременно в stdout и файл. Для Docker-сценария файловый вывод настраивается через `TELEGRAM_BOT_LOG_DIR`, а stdout остаётся доступен через `docker logs` и `docker compose logs`.

## Переменные окружения

- `TELEGRAM_BOT_TOKEN` – токен Telegram-бота (обязательно)
- `TELEGRAM_BOT_DB_PATH` – путь к файлу базы данных (по умолчанию `tasks.db`)
- `TELEGRAM_BOT_LOG_DIR` – директория для файловых логов (по умолчанию `out/logs`)
- `TELEGRAM_BOT_LONG_POLL_TIMEOUT_SEC` – timeout long polling в секундах (по умолчанию `1`, допустимый диапазон `1..50`)

## Деплой через GHCR

В репозитории настроен workflow [docker.yml](.github/workflows/docker.yml), который:

- собирает test target и прогоняет `ctest`
- публикует runtime-образ в GitHub Container Registry для `main` и тегов `v*`

На сервере достаточно выполнить вход в GHCR и указать нужный тег образа в `.env`:

```bash
echo "$GITHUB_TOKEN" | docker login ghcr.io -u USERNAME --password-stdin
./scripts/deploy_docker.sh
```

Проверка после деплоя:

```bash
docker compose -f deploy/docker/compose.yml --env-file deploy/docker/.env ps
docker compose -f deploy/docker/compose.yml --env-file deploy/docker/.env logs --tail=100
docker volume inspect check-list-bot_check-list-bot-data
```

## Graceful Shutdown

Бот корректно обрабатывает `SIGINT` и `SIGTERM`: завершает long polling, выходит из основного цикла и штатно освобождает ресурсы.

Максимальная задержка остановки примерно ограничена значением `TELEGRAM_BOT_LONG_POLL_TIMEOUT_SEC`, поэтому для запуска в Docker и на сервере можно настраивать баланс между отзывчивостью shutdown и частотой long polling без пересборки бинарника.

## Лицензия

Проект распространяется под лицензией MIT. Подробности см. в файле LICENSE (если файл отсутствует, его можно создать по шаблону MIT).

## Контакты

Автор: [Joraslav](mailto:yar.mozg2002@gmail.com)

GitHub: [https://github.com/Joraslav/check-list-bot](https://github.com/Joraslav/check-list-bot)
