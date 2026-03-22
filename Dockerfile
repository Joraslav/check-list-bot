FROM ubuntu:24.04 AS build

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    software-properties-common \
    apt-transport-https \
    ca-certificates \
    gnupg \
    lsb-release \
    python3 \
    python3-pip \
    python3-venv \
    curl && \
    rm -rf /var/lib/apt/lists/*

RUN curl -sSL https://install.python-poetry.org | python3 -
ENV PATH="/root/.local/bin:$PATH"

WORKDIR /app

COPY pyproject.toml poetry.lock /app/
COPY conanfile.txt /app/
COPY profiles/ /app/profiles

RUN poetry config virtualenvs.in-project true
RUN poetry install --with dev
RUN poetry run conan install . --build=missing -pr:h profiles/Release -pr:b profiles/Release

COPY . /app/

RUN poetry run cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release
RUN poetry run cmake --build build --config Release
RUN strip /app/build/check-list-bot || true

FROM build AS test
CMD ["/bin/bash", "-lc", "poetry run ctest --test-dir build -C Release --output-on-failure --output-junit /app/reports/ctest-report.xml"]

FROM ubuntu:24.04 AS runtime

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    ca-certificates \
    libstdc++6 && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /app/build/check-list-bot /app/check-list-bot

CMD ["./check-list-bot"]
