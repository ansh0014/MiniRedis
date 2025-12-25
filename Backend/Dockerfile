FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    pkg-config \
    libssl-dev \
    libpq-dev \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

# Install Drogon framework
WORKDIR /tmp
RUN git clone https://github.com/drogonframework/drogon.git && \
    cd drogon && \
    git submodule update --init && \
    mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc) && \
    make install && \
    ldconfig

# Install hiredis WITH CMAKE (FIXED: was using make before)
RUN git clone https://github.com/redis/hiredis.git /tmp/hiredis && \
    cd /tmp/hiredis && \
    mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DENABLE_SSL=ON \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          .. && \
    make -j$(nproc) && \
    make install && \
    ldconfig

# Install redis-plus-plus
RUN git clone https://github.com/sewenew/redis-plus-plus.git /tmp/redis-plus-plus && \
    cd /tmp/redis-plus-plus && \
    mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DREDIS_PLUS_PLUS_CXX_STANDARD=17 \
          -DCMAKE_PREFIX_PATH=/usr/local \
          .. && \
    make -j$(nproc) && \
    make install && \
    ldconfig

# Clean up build artifacts
RUN rm -rf /tmp/drogon /tmp/hiredis /tmp/redis-plus-plus

# Copy source code
WORKDIR /app
COPY . .

# Build the backend
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_PREFIX_PATH="/usr/local/lib/cmake;/usr/local" \
          .. && \
    make -j$(nproc)

# ========================================
# Runtime stage
# ========================================
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libpq5 \
    libjsoncpp25 \
    uuid-runtime \
    libssl3 \
    libboost-system1.74.0 \
    libboost-filesystem1.74.0 \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Copy libraries and binary from builder
COPY --from=builder /usr/local/lib /usr/local/lib
COPY --from=builder /app/build/miniredis_backend /app/miniredis_backend

RUN ldconfig

WORKDIR /app

# Create logs directory
RUN mkdir -p logs

EXPOSE 5500

# Run the backend
CMD ["./miniredis_backend"]