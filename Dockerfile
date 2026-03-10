FROM ubuntu:latest AS build

RUN apt-get update && \
    apt-get install -y \
    software-properties-common \
    apt-transport-https \
    ca-certificates \
    gnupg \
    lsb-release \
    python3 \
    python3-pip \
    python3-venv \
    curl

RUN curl -sSL https://install.python-poetry.org | python3 -
ENV PATH="/root/.local/bin:$PATH"

COPY pyproject.toml poetry.lock /app/

COPY conanfile.txt /app/
COPY profiles/ /app/profiles

WORKDIR /app

RUN poetry config virtualenvs.in-project true

RUN poetry install --with dev
RUN poetry run conan install . --build=missing -pr:h profiles/Release -pr:b profiles/Release

COPY . /app/

RUN poetry run cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release
RUN poetry run cmake --build build --config Release
