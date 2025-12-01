FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy only source files
COPY src/main.cpp ./src/
COPY src/TenantManager.h ./src/

# Generate CMakeLists.txt using echo
RUN echo "cmake_minimum_required(VERSION 3.10)" > CMakeLists.txt && \
    echo "project(MiniRedis CXX)" >> CMakeLists.txt && \
    echo "set(CMAKE_CXX_STANDARD 17)" >> CMakeLists.txt && \
    echo "set(CMAKE_CXX_STANDARD_REQUIRED ON)" >> CMakeLists.txt && \
    echo "add_executable(MiniRedis src/main.cpp)" >> CMakeLists.txt && \
    echo "target_link_libraries(MiniRedis pthread)" >> CMakeLists.txt

# Build
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# ========================================
# Runtime stage
# ========================================
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    netcat-traditional \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/build/MiniRedis /app/MiniRedis

WORKDIR /app

EXPOSE 6379

ENTRYPOINT ["./MiniRedis"]
CMD ["--port", "6379", "--tenant", "default"]