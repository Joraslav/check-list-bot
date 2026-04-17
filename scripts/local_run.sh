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
  --binary <path>     Path to built executable (default: build/check-list-bot)
  -h, --help          Show this help

Required environment variable:
  TELEGRAM_BOT_TOKEN

Optional environment variables:
  TELEGRAM_BOT_DB_PATH                 (default: tasks.db)
  TELEGRAM_BOT_LOG_DIR                 (default: out/logs)
  TELEGRAM_BOT_LONG_POLL_TIMEOUT_SEC   (default: 1)

Examples:
  $(basename "$0")
  $(basename "$0") --env-file .env.local
  $(basename "$0") -- --verbose
EOF
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
BINARY_PATH="${DEFAULT_BINARY}"
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

DB_DIR="$(dirname "${TELEGRAM_BOT_DB_PATH}")"
mkdir -p "${PROJECT_ROOT}/${DB_DIR}" "${PROJECT_ROOT}/${TELEGRAM_BOT_LOG_DIR}"

if [ ! -x "$BINARY_PATH" ]; then
	error_exit "Built binary not found or not executable: ${BINARY_PATH}. Build project first (cmake --preset conan-debug && cmake --build --preset conan-debug)."
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
	"$BINARY_PATH" "${BOT_ARGS[@]}"
