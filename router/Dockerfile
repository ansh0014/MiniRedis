FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy only source files
COPY src/MiniRouter.cpp ./src/

# Generate CMakeLists.txt using echo
RUN echo "cmake_minimum_required(VERSION 3.10)" > CMakeLists.txt && \
    echo "project(MiniRouter CXX)" >> CMakeLists.txt && \
    echo "set(CMAKE_CXX_STANDARD 17)" >> CMakeLists.txt && \
    echo "set(CMAKE_CXX_STANDARD_REQUIRED ON)" >> CMakeLists.txt && \
    echo "add_executable(MiniRouter src/MiniRouter.cpp)" >> CMakeLists.txt && \
    echo "target_link_libraries(MiniRouter pthread curl)" >> CMakeLists.txt

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
    curl \
    libcurl4 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/build/MiniRouter /app/MiniRouter

WORKDIR /app

EXPOSE 6300

CMD ["./MiniRouter"]
