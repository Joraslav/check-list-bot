#!/bin/bash

set -euo pipefail

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
COMPOSE_FILE="${PROJECT_ROOT}/deploy/docker/compose.yml"
ENV_EXAMPLE_FILE="${PROJECT_ROOT}/deploy/docker/.env.example"
ENV_FILE="${PROJECT_ROOT}/deploy/docker/.env"

# Allow overriding compose command if needed (for example, podman-compose wrapper).
COMPOSE_CMD=${COMPOSE_CMD:-"docker compose"}
TELEGRAM_API_BASE="https://api.telegram.org"
CONNECT_TIMEOUT_SEC=5
MAX_TIME_SEC=15

error_exit() {
    echo -e "${RED}Ошибка: $1${NC}" >&2
    exit 1
}

require_compose() {
    if ! ${COMPOSE_CMD} version >/dev/null 2>&1; then
        error_exit "Команда '${COMPOSE_CMD}' недоступна. Установите Docker Compose или задайте COMPOSE_CMD."
    fi
}

require_tool() {
    local tool_name="$1"
    command -v "$tool_name" >/dev/null 2>&1 ||
        error_exit "Не найдено требуемое приложение в PATH: ${tool_name}"
}

ensure_env_file() {
    if [ ! -f "${ENV_FILE}" ]; then
        cp "${ENV_EXAMPLE_FILE}" "${ENV_FILE}" || error_exit "Не удалось создать ${ENV_FILE}"
        echo -e "${YELLOW}Создан файл ${ENV_FILE} из шаблона. Заполните TELEGRAM_BOT_TOKEN.${NC}"
    fi

    if grep -q '^TELEGRAM_BOT_TOKEN=replace-with-real-bot-token$' "${ENV_FILE}"; then
        error_exit "В ${ENV_FILE} не задан реальный TELEGRAM_BOT_TOKEN"
    fi
}

compose() {
    ${COMPOSE_CMD} -f "${COMPOSE_FILE}" --env-file "${ENV_FILE}" "$@"
}

load_env_file() {
    set -a
    # shellcheck disable=SC1090
    source "${ENV_FILE}"
    set +a
}

preflight_check_telegram_api() {
    require_tool curl
    load_env_file

    if [ -z "${TELEGRAM_BOT_TOKEN:-}" ]; then
        error_exit "TELEGRAM_BOT_TOKEN отсутствует в ${ENV_FILE}"
    fi

    local response=""
    response="$(curl -4 -L -sS --connect-timeout "${CONNECT_TIMEOUT_SEC}" --max-time "${MAX_TIME_SEC}" \
        "${TELEGRAM_API_BASE}/bot${TELEGRAM_BOT_TOKEN}/getMe")" || {
        error_exit "Preflight getMe не прошел (network/timeout)."
    }

    if [[ "$response" != *'"ok":true'* ]]; then
        error_exit "Preflight getMe вернул неуспешный ответ: ${response}"
    fi

    echo -e "${GREEN}Preflight getMe прошел успешно (ok=true).${NC}"
}

deploy() {
    ensure_env_file
    preflight_check_telegram_api
    echo -e "${YELLOW}Pull образа...${NC}"
    compose pull
    echo -e "${YELLOW}Запуск сервиса...${NC}"
    compose up -d
    compose ps
    echo -e "${GREEN}Деплой завершен.${NC}"
}

show_logs() {
    ensure_env_file
    compose logs -f
}

restart_service() {
    ensure_env_file
    compose restart
    compose ps
}

status() {
    ensure_env_file
    compose ps
}

stop_service() {
    ensure_env_file
    compose down
}

usage() {
    cat <<EOF
Usage: $(basename "$0") [command]

Commands:
  deploy   Pull + up -d + ps (default)
  logs     Follow logs
  restart  Restart service
  status   Show service status
  down     Stop and remove containers
  help     Show this help

Environment:
  COMPOSE_CMD  Override compose command (default: "docker compose")
EOF
}

require_compose

case "${1:-deploy}" in
    deploy)
        deploy
        ;;
    logs)
        show_logs
        ;;
    restart)
        restart_service
        ;;
    status)
        status
        ;;
    down)
        stop_service
        ;;
    help|-h|--help)
        usage
        ;;
    *)
        usage
        error_exit "Неизвестная команда: $1"
        ;;
esac
