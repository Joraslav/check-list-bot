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

Соберите образ:

```bash
docker build -t check-list-bot .
```

Запустите контейнер:

```bash
docker run \
   -e TELEGRAM_BOT_TOKEN="your-telegram-bot-token" \
   -e TELEGRAM_BOT_DB_PATH="/data/tasks.db" \
   -e TELEGRAM_BOT_LONG_POLL_TIMEOUT_SEC="1" \
   -v $(pwd)/data:/data \
   check-list-bot
```

Контейнер также содержит дефолтное значение `TELEGRAM_BOT_LONG_POLL_TIMEOUT_SEC=1`, поэтому переменную нужно передавать только если вы хотите изменить скорость завершения long polling.

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
├── .github/workflows/ – CI/CD (форматирование, сборка)
├── CMakeLists.txt     – корневой CMake
├── conanfile.txt      – зависимости Conan
├── pyproject.toml     – конфигурация Poetry (для dev-инструментов)
├── main.cpp           – точка входа
└── Dockerfile         – конфигурация Docker
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

Для Docker-сценария можно использовать stage `test` из `Dockerfile`.

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

Логи пишутся в stdout и файл. Уровень логирования сейчас задаётся в коде, отдельная runtime-переменная окружения для него не реализована.

## Переменные окружения

- `TELEGRAM_BOT_TOKEN` – токен Telegram-бота (обязательно)
- `TELEGRAM_BOT_DB_PATH` – путь к файлу базы данных (по умолчанию `tasks.db`)
- `TELEGRAM_BOT_LONG_POLL_TIMEOUT_SEC` – timeout long polling в секундах (по умолчанию `1`, допустимый диапазон `1..50`)

## Graceful Shutdown

Бот корректно обрабатывает `SIGINT` и `SIGTERM`: завершает long polling, выходит из основного цикла и штатно освобождает ресурсы.

Максимальная задержка остановки примерно ограничена значением `TELEGRAM_BOT_LONG_POLL_TIMEOUT_SEC`, поэтому для запуска в Docker и на сервере можно настраивать баланс между отзывчивостью shutdown и частотой long polling без пересборки бинарника.

## Лицензия

Проект распространяется под лицензией MIT. Подробности см. в файле LICENSE (если файл отсутствует, его можно создать по шаблону MIT).

## Контакты

Автор: [Joraslav](mailto:yar.mozg2002@gmail.com)

GitHub: [https://github.com/Joraslav/check-list-bot](https://github.com/Joraslav/check-list-bot)
