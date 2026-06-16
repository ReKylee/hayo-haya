FROM debian:bookworm

RUN apt-get update \
	&& apt-get install -y --no-install-recommends \
		build-essential \
		clang \
		cmake \
		ninja-build \
		bison \
		re2c \
		ca-certificates \
	&& rm -rf /var/lib/apt/lists/*

WORKDIR /work
