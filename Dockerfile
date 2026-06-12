FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    gdb \
    cmake \
    ninja-build \
    bison \
    re2c \
    git \
    python3 \
    locales \
    && rm -rf /var/lib/apt/lists/*

RUN locale-gen C.UTF-8 || true

ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

WORKDIR /work
