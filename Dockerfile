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

# RUN curl -sSL https://install.python-poetry.org | python3 -
RUN pip3 install --no-cache-dir poetry

COPY pyproject.toml poetry.lock /app/

COPY conanfile.txt /app/
COPY profiles/ /app/profiles

WORKDIR /app

RUN poetry config virtualenvs.in-project true

RUN poetry install --with dev
RUN poetry run conan install . --build=missing -pr:h profiles/Release -pr:b profiles/Release

COPY . /app/

RUN cmake --preset conan-release
RUN cmake --build --preset conan-release
