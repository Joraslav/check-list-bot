#!/bin/bash

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# Функция для обработки ошибок
error_exit() {
    echo -e "Ошибка: $1"
    exit 1
}

echo -e "${YELLOW}Очистка рабочей области...${NC}"

if [ -d "build" ]; then
    rm -rf build || error_exit "${RED}Не удалось удалить папку build${NC}"
fi

if [ -f "CMakeUserPresets.json" ]; then
    rm CMakeUserPresets.json || error_exit "${RED}Не удалось удалить файл CMakeUserPresets.json${NC}"
fi

if [ -d "out" ]; then
    find out -mindepth 1 -delete || error_exit "${RED}Не удалось очистить папку out${NC}"
fi

if [ -d ".venv" ]; then
    rm -rf .venv || error_exit "${RED}Не удалось удалить папку .venv${NC}"
fi

echo -e "${GREEN}Очистка завершена!${NC}"