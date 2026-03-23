#!/bin/bash

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

if [ $# -eq 0 ]; then
    echo -e "${RED}Ошибка: Не указан тип сборки${NC}"
    echo "Использование: $0 [Debug|Release]"
    exit 1
fi

BUILD_TYPE=$1

if [ "$BUILD_TYPE" != "Debug" ] && [ "$BUILD_TYPE" != "Release" ]; then
    echo -e "${YELLOW}Ошибка: Неверный тип сборки '$BUILD_TYPE'. Допустимые значения: Debug, Release${NC}"
    exit 1
fi

echo -e "${GREEN}Настройка окружения проекта для сборки типа $BUILD_TYPE...${NC}"

echo -e "${GREEN}Установка зависимостей Conan...${NC}"
conan install . --build=missing -pr:h profiles/$BUILD_TYPE -pr:b profiles/$BUILD_TYPE

echo -e "${GREEN}Окружение успешно настроено!${NC}"
