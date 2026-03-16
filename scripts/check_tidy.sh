#!/bin/bash

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

LIST_ONLY=false
APPLY_FIXES=false
BUILD_DIR="build"
BUILD_TYPE=""

print_usage() {
	echo "Использование: $0 [--list] [--fix] [--build-dir <path>] [--build-type <Debug|Release|...>]"
}

while [ $# -gt 0 ]; do
	case "$1" in
		--list)
			LIST_ONLY=true
			shift
			;;
		--fix)
			APPLY_FIXES=true
			shift
			;;
		--build-dir)
			if [ -z "$2" ]; then
				echo -e "${RED}✗ Не указан путь для --build-dir.${NC}"
				print_usage
				exit 1
			fi
			BUILD_DIR="$2"
			shift 2
			;;
		--build-type)
			if [ -z "$2" ]; then
				echo -e "${RED}✗ Не указан тип сборки для --build-type.${NC}"
				print_usage
				exit 1
			fi
			BUILD_TYPE="$2"
			shift 2
			;;
		--help)
			print_usage
			exit 0
			;;
		*)
			echo -e "${RED}✗ Неизвестный аргумент: $1${NC}"
			print_usage
			exit 1
			;;
	esac
done

if ! command -v clang-tidy &> /dev/null; then
	echo -e "${RED}✗ clang-tidy не найден. Установите его и повторите попытку.${NC}"
	exit 1
fi

if ! command -v cmake &> /dev/null; then
	echo -e "${RED}✗ cmake не найден. Установите его и повторите попытку.${NC}"
	exit 1
fi

echo -e "${YELLOW}Поиск C++ файлов для проверки...${NC}"

mapfile -t cpp_files < <(
	if [ -d "./src" ]; then
		find ./src -type f \
			\( -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" -o -name "*.c" \)
	fi

	if [ -f "./main.cpp" ]; then
		echo "./main.cpp"
	fi
)

if [ ${#cpp_files[@]} -eq 0 ]; then
	echo -e "${YELLOW}C++ исходники не найдены.${NC}"
	exit 0
fi

if [ "$LIST_ONLY" = true ]; then
	echo -e "${YELLOW}Найдено ${#cpp_files[@]} файлов:${NC}"
	printf '%s\n' "${cpp_files[@]}"
	exit 0
fi

if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
	if [ -z "$BUILD_TYPE" ]; then
		echo -e "${RED}✗ compile_commands.json не найден в '$BUILD_DIR'.${NC}"
		echo -e "${RED}  Укажите --build-type для автоконфигурации или передайте уже сконфигурированный --build-dir.${NC}"
		exit 1
	fi

	echo -e "${YELLOW}compile_commands.json не найден в '$BUILD_DIR'. Запускаю конфигурацию CMake (CMAKE_BUILD_TYPE=$BUILD_TYPE)...${NC}"
	cmake -S . -B "$BUILD_DIR" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
fi

if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
	echo -e "${RED}✗ Не удалось найти или создать $BUILD_DIR/compile_commands.json${NC}"
	exit 1
fi

echo -e "${YELLOW}Запускаю clang-tidy для ${#cpp_files[@]} файлов...${NC}"

clang_tidy_args=("-p" "$BUILD_DIR")

if [ "$APPLY_FIXES" = true ]; then
	clang_tidy_args+=("--fix" "--format-style=file")
	echo -e "${YELLOW}Включены автоисправления (--fix).${NC}"
fi

set +e
clang-tidy "${cpp_files[@]}" "${clang_tidy_args[@]}"
clang_tidy_exit_code=$?
set -e

if [ $clang_tidy_exit_code -eq 0 ]; then
	echo -e "${GREEN}✓ Проверка clang-tidy завершена успешно!${NC}"
else
	echo -e "${RED}✗ clang-tidy обнаружил замечания. Код возврата: $clang_tidy_exit_code${NC}"
	exit $clang_tidy_exit_code
fi