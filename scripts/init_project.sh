#!/bin/bash

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${YELLOW}Настройка и проверка Poetry...${NC}"

if [ ! -f "pyproject.toml" ]; then
    echo -e "${RED}✗ Файл pyproject.toml не найден!${NC}"
    exit 1
fi

if ! command -v poetry &> /dev/null; then
    echo -e "${RED}✗ Poetry не найден! Установите poetry:${NC}"
    echo "apt-get install python3-poetry"
    exit 1
fi

echo -e "${GREEN}✓ Найден pyproject.toml${NC}"
echo -e "${GREEN}✓ Найден Poetry: $(poetry --version)${NC}"
echo

poetry config virtualenvs.in-project true

echo -e "${YELLOW}Создание виртуального окружения Python...${NC}"

if [ -d ".venv" ]; then
    echo -e "${YELLOW}Удаляем существующую папку .venv...${NC}"
    rm -rf .venv
fi

python3 -m venv .venv

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Виртуальное окружение успешно создано в папке .venv${NC}"
else
    echo -e "${RED}✗ Ошибка при создании виртуального окружения${NC}"
    exit 1
fi

echo -e "${YELLOW}Инсталляция зависимостей...${NC}"

poetry install

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Виртуальное окружение успешно настроенно${NC}"
else
    echo -e "${RED}✗ Ошибка при настройке виртуального окружения${NC}"
    exit 1
fi

echo -e "${YELLOW}Настройка и установка библиотек С++...${NC}"

poetry run conan install . --build=missing -pr:h profiles/Debug -pr:b profiles/Debug

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Виртуальное окружение успешно установлено и настроено${NC}"
    echo -e "${YELLOW}Для активации окружения выполните:${NC}"
    echo -e "${GREEN}source .venv/bin/activate${NC}"
else
    echo -e "${RED}✗ Ошибка при настройке виртуального окружения${NC}"
    exit 1
fi