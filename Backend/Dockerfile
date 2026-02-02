FROM drogon-base:latest AS builder

WORKDIR /app

COPY Backend/CMakeLists.txt .
COPY Backend/main.cc .
COPY Backend/controller/ controller/
COPY config/ config/


RUN mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libpq5 libssl3 libjsoncpp25 libhiredis0.14 \
    libbrotli1 zlib1g libc-ares2 uuid-runtime \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/lib/libdrogon* /usr/local/lib/
COPY --from=builder /usr/local/lib/libredis++* /usr/local/lib/
COPY --from=builder /usr/local/lib/libhiredis* /usr/local/lib/
COPY --from=builder /usr/local/include/drogon /usr/local/include/drogon
COPY --from=builder /usr/local/include/hiredis /usr/local/include/hiredis

RUN ldconfig

WORKDIR /app

COPY --from=builder /app/build/miniredis_backend .

EXPOSE 5500

CMD ["./miniredis_backend"]
