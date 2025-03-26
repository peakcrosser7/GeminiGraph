FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y \
    build-essential \
    mpich \
    libnuma-dev \
    python3 \
    gdb


CMD ["/bin/bash"]
