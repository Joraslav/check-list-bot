#!/bin/sh
set -eu

: "${TELEGRAM_BOT_DB_PATH:=/app/data/tasks.db}"
: "${TELEGRAM_BOT_LOG_DIR:=/app/logs}"

db_directory=$(dirname "$TELEGRAM_BOT_DB_PATH")

[ -n "${TELEGRAM_BOT_TOKEN:-}" ]
[ -d "$db_directory" ]
[ -d "$TELEGRAM_BOT_LOG_DIR" ]

kill -0 1