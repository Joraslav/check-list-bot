#!/bin/bash

set -euo pipefail

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

DEFAULT_BINARY="${PROJECT_ROOT}/build/check-list-bot"
DEFAULT_ENV_FILE="${PROJECT_ROOT}/.env.local"
DEFAULT_BUILD_TYPE="Debug"

TELEGRAM_API_BASE="https://api.telegram.org"
CONNECT_TIMEOUT_SEC=5
MAX_TIME_SEC=15

error_exit() {
	echo -e "${RED}Error: $1${NC}" >&2
	exit 1
}

usage() {
	cat <<EOF
Usage: $(basename "$0") [options] [-- <bot_args>]

Run check-list-bot locally (native mode, no container).

Options:
  --env-file <path>   Path to env file with TELEGRAM_BOT_* vars (default: .env.local if exists)
	--binary <path>     Path to built executable (overrides auto-detection)
	--build-type <type> Build profile for automation/autodetect: Debug or Release (default: Debug)
	--prepare           Run create_env + cmake configure + cmake build before launch
	--check-api         Validate token via getMe before launch
	--clear-webhook     Call deleteWebhook?drop_pending_updates=true before launch
	--show-http-client  Print current TgBot default client implementation from Conan sources
  -h, --help          Show this help

Required environment variable:
  TELEGRAM_BOT_TOKEN

Optional environment variables:
  TELEGRAM_BOT_DB_PATH                 (default: tasks.db)
  TELEGRAM_BOT_LOG_DIR                 (default: out/logs)
  TELEGRAM_BOT_LONG_POLL_TIMEOUT_SEC   (default: 1)
	TELEGRAM_BOT_PROXY                   (example: socks5h://127.0.0.1:10808)
	TELEGRAM_BOT_CLEAR_WEBHOOK_ON_START  (default: false)
	TELEGRAM_BOT_WEBHOOK_CLEANUP_MAX_ATTEMPTS (default: 3)

Examples:
  $(basename "$0")
	$(basename "$0") --prepare --build-type Debug --check-api --clear-webhook
  $(basename "$0") --env-file .env.local
	$(basename "$0") --build-type Release -- --verbose
EOF
}

require_tool() {
	local tool_name="$1"
	command -v "$tool_name" >/dev/null 2>&1 || error_exit "Required tool not found in PATH: ${tool_name}"
}

resolve_binary_path() {
	local preferred_type="$1"
	local primary="${PROJECT_ROOT}/build/check-list-bot"
	local fallback_type=""
	if [ "$preferred_type" = "Debug" ]; then
		fallback_type="Release"
	else
		fallback_type="Debug"
	fi

	local candidate1="${PROJECT_ROOT}/build/${preferred_type}/check-list-bot"
	local candidate2="${PROJECT_ROOT}/build/${fallback_type}/check-list-bot"

	if [ -x "$primary" ]; then
		echo "$primary"
		return 0
	fi

	if [ -x "$candidate1" ]; then
		echo "$candidate1"
		return 0
	fi

	if [ -x "$candidate2" ]; then
		echo "$candidate2"
		return 0
	fi

	error_exit "Built binary not found. Tried: ${primary}, ${candidate1}, ${candidate2}."
}

run_prepare() {
	local build_type="$1"
	local preset_type=""
	if [ "$build_type" = "Debug" ]; then
		preset_type="debug"
	else
		preset_type="release"
	fi

	echo -e "${YELLOW}Preparing local build for ${build_type}...${NC}"
	"${PROJECT_ROOT}/scripts/create_env.sh" "$build_type"
	cmake --preset "conan-${preset_type}"
	cmake --build --preset "conan-${preset_type}"
}

run_getme_check() {
	require_tool curl
	local response=""
	response="$(curl -4 -L -sS --connect-timeout "${CONNECT_TIMEOUT_SEC}" --max-time "${MAX_TIME_SEC}" \
		"${TELEGRAM_API_BASE}/bot${TELEGRAM_BOT_TOKEN}/getMe")" || {
		error_exit "Telegram API check failed (network/timeout)."
	}

	if [[ "$response" != *'"ok":true'* ]]; then
		error_exit "Telegram token check failed. Response: ${response}"
	fi

	echo -e "${GREEN}Telegram API check passed (getMe ok=true).${NC}"
}

clear_webhook() {
	require_tool curl
	local response=""
	response="$(curl -4 -L -sS --connect-timeout "${CONNECT_TIMEOUT_SEC}" --max-time "${MAX_TIME_SEC}" \
		"${TELEGRAM_API_BASE}/bot${TELEGRAM_BOT_TOKEN}/deleteWebhook?drop_pending_updates=true")" || {
		error_exit "Failed to clear webhook (network/timeout)."
	}

	if [[ "$response" != *'"ok":true'* ]]; then
		error_exit "deleteWebhook failed. Response: ${response}"
	fi

	echo -e "${GREEN}Webhook cleared successfully.${NC}"
}

show_tgbot_default_client() {
	local data_file="${PROJECT_ROOT}/build/Debug/generators/tgbot-debug-x86_64-data.cmake"
	if [ ! -f "$data_file" ]; then
		data_file="${PROJECT_ROOT}/build/Release/generators/tgbot-release-x86_64-data.cmake"
	fi

	if [ ! -f "$data_file" ]; then
		echo -e "${YELLOW}No tgbot CMakeDeps file found yet. Run create_env.sh first.${NC}"
		return 0
	fi

	local package_folder=""
	package_folder="$(awk -F '"' '/set\(tgbot_PACKAGE_FOLDER_(DEBUG|RELEASE) /{print $2; exit}' "$data_file")"
	if [ -z "$package_folder" ]; then
		echo -e "${YELLOW}Cannot detect tgbot package folder from ${data_file}.${NC}"
		return 0
	fi

	local bot_cpp="${package_folder}/include/tgbot/Bot.h"
	if [ ! -f "$bot_cpp" ]; then
		echo -e "${YELLOW}Cannot read ${bot_cpp}.${NC}"
		return 0
	fi

	echo -e "${GREEN}tgbot package folder: ${package_folder}${NC}"
	echo -e "${GREEN}Bot constructor default client is selected in library source (usually BoostHttpOnlySslClient by default).${NC}"
}

load_env_file() {
	local env_file="$1"
	if [ ! -f "$env_file" ]; then
		error_exit "Env file not found: $env_file"
	fi

	echo -e "${YELLOW}Loading env file: ${env_file}${NC}"
	set -a
	# shellcheck disable=SC1090
	source "$env_file"
	set +a
}

ENV_FILE=""
BINARY_PATH=""
BUILD_TYPE="${DEFAULT_BUILD_TYPE}"
DO_PREPARE=0
DO_CHECK_API=0
DO_CLEAR_WEBHOOK=0
DO_SHOW_HTTP_CLIENT=0
BOT_ARGS=()

while [ "$#" -gt 0 ]; do
	case "$1" in
		--env-file)
			[ "$#" -ge 2 ] || error_exit "--env-file requires a value"
			ENV_FILE="$2"
			shift 2
			;;
		--binary)
			[ "$#" -ge 2 ] || error_exit "--binary requires a value"
			BINARY_PATH="$2"
			shift 2
			;;
		--build-type)
			[ "$#" -ge 2 ] || error_exit "--build-type requires a value"
			BUILD_TYPE="$2"
			shift 2
			;;
		--prepare)
			DO_PREPARE=1
			shift
			;;
		--check-api)
			DO_CHECK_API=1
			shift
			;;
		--clear-webhook)
			DO_CLEAR_WEBHOOK=1
			shift
			;;
		--show-http-client)
			DO_SHOW_HTTP_CLIENT=1
			shift
			;;
		--)
			shift
			BOT_ARGS=("$@")
			break
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			error_exit "Unknown option: $1"
			;;
	esac
done

if [ "$BUILD_TYPE" != "Debug" ] && [ "$BUILD_TYPE" != "Release" ]; then
	error_exit "Invalid --build-type '${BUILD_TYPE}'. Allowed values: Debug, Release."
fi

if [ -n "$ENV_FILE" ]; then
	load_env_file "$ENV_FILE"
elif [ -f "$DEFAULT_ENV_FILE" ]; then
	load_env_file "$DEFAULT_ENV_FILE"
fi

if [ -z "${TELEGRAM_BOT_TOKEN:-}" ]; then
	error_exit "TELEGRAM_BOT_TOKEN is not set. Export it or provide --env-file."
fi

: "${TELEGRAM_BOT_DB_PATH:=tasks.db}"
: "${TELEGRAM_BOT_LOG_DIR:=out/logs}"
: "${TELEGRAM_BOT_LONG_POLL_TIMEOUT_SEC:=1}"
: "${TELEGRAM_BOT_CLEAR_WEBHOOK_ON_START:=false}"
: "${TELEGRAM_BOT_WEBHOOK_CLEANUP_MAX_ATTEMPTS:=3}"

DB_DIR="$(dirname "${TELEGRAM_BOT_DB_PATH}")"
mkdir -p "${PROJECT_ROOT}/${DB_DIR}" "${PROJECT_ROOT}/${TELEGRAM_BOT_LOG_DIR}"

if [ "$DO_SHOW_HTTP_CLIENT" -eq 1 ]; then
	show_tgbot_default_client
fi

if [ "$DO_PREPARE" -eq 1 ]; then
	run_prepare "$BUILD_TYPE"
fi

if [ -z "$BINARY_PATH" ]; then
	BINARY_PATH="$(resolve_binary_path "$BUILD_TYPE")"
fi

if [ "$DO_CHECK_API" -eq 1 ]; then
	run_getme_check
fi

if [ "$DO_CLEAR_WEBHOOK" -eq 1 ]; then
	clear_webhook
fi

if [ ! -x "$BINARY_PATH" ]; then
	error_exit "Built binary not found or not executable: ${BINARY_PATH}."
fi

echo -e "${GREEN}Starting bot locally...${NC}"
echo -e "${GREEN}Binary: ${BINARY_PATH}${NC}"
echo -e "${GREEN}DB: ${TELEGRAM_BOT_DB_PATH}${NC}"
echo -e "${GREEN}Logs: ${TELEGRAM_BOT_LOG_DIR}${NC}"

cd "$PROJECT_ROOT"
exec env \
	TELEGRAM_BOT_TOKEN="${TELEGRAM_BOT_TOKEN}" \
	TELEGRAM_BOT_DB_PATH="${TELEGRAM_BOT_DB_PATH}" \
	TELEGRAM_BOT_LOG_DIR="${TELEGRAM_BOT_LOG_DIR}" \
	TELEGRAM_BOT_LONG_POLL_TIMEOUT_SEC="${TELEGRAM_BOT_LONG_POLL_TIMEOUT_SEC}" \
	TELEGRAM_BOT_PROXY="${TELEGRAM_BOT_PROXY:-}" \
	TELEGRAM_BOT_CLEAR_WEBHOOK_ON_START="${TELEGRAM_BOT_CLEAR_WEBHOOK_ON_START}" \
	TELEGRAM_BOT_WEBHOOK_CLEANUP_MAX_ATTEMPTS="${TELEGRAM_BOT_WEBHOOK_CLEANUP_MAX_ATTEMPTS}" \
	"$BINARY_PATH" "${BOT_ARGS[@]}"
