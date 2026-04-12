#!/bin/sh
set -eu

if [ -z "${TELEGRAM_BOT_TOKEN:-}" ]; then
    echo "Error: TELEGRAM_BOT_TOKEN environment variable is not set" >&2
    exit 1
fi

: "${TELEGRAM_BOT_DB_PATH:=/app/data/tasks.db}"
: "${TELEGRAM_BOT_LOG_DIR:=/app/logs}"

db_directory=$(dirname "$TELEGRAM_BOT_DB_PATH")

mkdir -p "$db_directory" "$TELEGRAM_BOT_LOG_DIR"

touch "$TELEGRAM_BOT_LOG_DIR/.write-test"
rm -f "$TELEGRAM_BOT_LOG_DIR/.write-test"

exec "$@"