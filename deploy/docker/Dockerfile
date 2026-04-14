# syntax=docker/dockerfile:1.7

FROM ubuntu:24.04 AS build

ENV DEBIAN_FRONTEND=noninteractive
ENV PATH="/root/.local/bin:$PATH"
ENV POETRY_NO_INTERACTION=1
ENV POETRY_VIRTUALENVS_IN_PROJECT=true

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt/lists,sharing=locked \
    apt-get update && \
    apt-get install -y --no-install-recommends \
    software-properties-common \
    apt-transport-https \
    ca-certificates \
    gnupg \
    lsb-release \
    build-essential \
    pkg-config \
    git \
    python3 \
    python3-pip \
    python3-venv \
    curl && \
    rm -rf /var/lib/apt/lists/*

RUN curl -sSL https://install.python-poetry.org | python3 -

WORKDIR /app

COPY pyproject.toml poetry.lock README.md /app/
COPY conanfile.txt /app/
COPY profiles/ /app/profiles

RUN --mount=type=cache,target=/root/.cache/pypoetry,sharing=locked \
    --mount=type=cache,target=/root/.cache/pip,sharing=locked \
    poetry install --only dev --no-root
RUN poetry run conan install . --build=missing -pr:h profiles/Release -pr:b profiles/Release

COPY CMakeLists.txt /app/
COPY cmake/ /app/cmake
COPY main.cpp /app/
COPY src/ /app/src
COPY tests/ /app/tests

RUN poetry run cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_TESTS=ON
RUN poetry run cmake --build build --config Release
RUN strip /app/build/check-list-bot || true

FROM build AS test
RUN mkdir -p /app/reports
CMD ["/bin/bash", "-lc", "poetry run ctest --test-dir build -C Release --output-on-failure --output-junit /app/reports/ctest-report.xml"]

FROM ubuntu:24.04 AS runtime

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    ca-certificates \
    bash \
    libstdc++6 && \
    rm -rf /var/lib/apt/lists/*

RUN groupadd --system --gid 10001 app && \
    useradd --system --uid 10001 --gid app --home-dir /app --create-home --shell /usr/sbin/nologin app

WORKDIR /app
COPY --from=build /app/build/check-list-bot /app/check-list-bot
COPY deploy/docker/entrypoint.sh /usr/local/bin/docker-entrypoint.sh
COPY deploy/docker/healthcheck.sh /usr/local/bin/healthcheck.sh

RUN chmod 755 /usr/local/bin/docker-entrypoint.sh /usr/local/bin/healthcheck.sh && \
    mkdir -p /app/data /app/logs && \
    chown -R app:app /app

ENV TELEGRAM_BOT_DB_PATH=/app/data/tasks.db
ENV TELEGRAM_BOT_LOG_DIR=/app/logs
ENV TELEGRAM_BOT_LONG_POLL_TIMEOUT_SEC=1

VOLUME ["/app/data", "/app/logs"]

USER app

HEALTHCHECK --interval=30s --timeout=5s --start-period=15s --retries=3 \
    CMD ["/usr/local/bin/healthcheck.sh"]

ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
CMD ["./check-list-bot"]
