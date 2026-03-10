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
   git clone https://github.com/your-username/check-list-bot.git
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
   TOKEN="your-telegram-bot-token" ./check-list-bot
   ```

### Использование Docker

Соберите образ:

```bash
docker build -t check-list-bot .
```

Запустите контейнер:

```bash
docker run -e TOKEN="your-telegram-bot-token" check-list-bot
```

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

Проект использует clang-format. Проверить форматирование можно через скрипт:

```bash
./scripts/init_project.sh
```

Или вручную:

```bash
find src -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
```

### Тестирование

(Планируется добавить unit-тесты с помощью Catch2 или Google Test.)

### Логирование

Логи пишутся в stdout и файл (настраивается через переменные окружения). Уровень логирования можно изменить в коде.

## Переменные окружения

- `TOKEN` – токен Telegram-бота (обязательно)
- `LOG_LEVEL` – уровень логирования (info, warn, error, debug)
- `DB_PATH` – путь к файлу базы данных (по умолчанию tasks.db)

## Лицензия

Проект распространяется под лицензией MIT. Подробности см. в файле LICENSE (если файл отсутствует, его можно создать по шаблону MIT).

## Контакты

Автор: [Joraslav](mailto:yar.mozg2002@gmail.com)

GitHub: [https://github.com/Joraslav/check-list-bot](https://github.com/Joraslav/check-list-bot)