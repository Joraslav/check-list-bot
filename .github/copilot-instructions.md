# check-list-bot Project Guidelines

## Стек и окружение

- Язык: **C++23**, компилятор GCC 15
- Сборка: **CMake** + **Conan** (профили в `profiles/`)
- Линтер/форматтер: **clang-tidy** + **clang-format** (расширение clangd в VS Code)
- Зависимости: `tgbot`, `SQLiteCpp`, `spdlog`, `magic_enum`, `GTest`
- Логирование: `spdlog` через обёртку `logging::Log` (см. `src/log/`)
- Тесты: Google Test, собираются при `WITH_TESTS=ON`

## Структура проекта

- `src/bot/` — Telegram-бот: хендлеры, клавиатуры
- `src/database/` — слой БД: `TaskDB`, `ScopedStatement`, `StatementManager`, `SQL`
- `src/log/` — обёртка логирования
- `src/utils/` — утилиты
- `tests/` — юнит-тесты (GTest)

## Общие правила

- Следовать стилю и соглашениям из `.github/instructions/cpp-style.instructions.md` для C++-кода
- Следовать правилам из `.github/instructions/testing-style.instructions.md` при написании и изменении тестов
- Не использовать сырые указатели: предпочитать `std::unique_ptr`, `std::shared_ptr` или ссылки
- Перед каждым `// NOLINT(...)` писать короткое объяснение, почему подавление допустимо
- Все обращения к `std::getenv` выполнять только через обёртку `ReadEnvVar` из `main.cpp`
- Учитывать диагностику и форматирование `clangd`; не предлагать стиль, противоречащий `clang-format` и `clang-tidy`

## Сборка и проверка

- Для сборки использовать CMake и Conan-конфигурацию проекта
- При изменениях в C++-коде по возможности проверять сборку и релевантные тесты
- Не вводить новые зависимости без явной необходимости


