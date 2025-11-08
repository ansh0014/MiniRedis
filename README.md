# MiniRedis

MiniRedis is a minimal, single-file, in-memory Redis-like server implemented in C++ for Windows.  
It demonstrates basic TCP networking with Winsock and simple concurrency using std::thread and std::mutex.

## Features
- SET / GET / DEL / QUIT commands
- Simple Redis-like responses (RESP-ish)
- Concurrent clients via std::thread
- Small, educational codebase

## Requirements
- Windows 10/11
- MSVC (Visual Studio) or MinGW-w64
- C++17 or newer

## Build

MSVC (Developer Command Prompt):
```bat
cl /EHsc /std:c++17 main.cpp Ws2_32.lib /Fe:MiniRedis.exe
```

MinGW:
```bat
g++ -std=c++17 -O2 -o MiniRedis.exe main.cpp -lws2_32 -pthread
```

## Run
Start the server:
```bat
.\MiniRedis.exe
```
The server listens on TCP port 6379 by default.

## Usage / Examples
Connect with any TCP client (telnet, netcat, redis-cli) and send plain text commands (one per line):

telnet example:
```
telnet localhost 6379
SET key value
GET key
DEL key
QUIT
```

redis-cli (raw mode):
```
redis-cli -p 6379 --raw
SET hello world
GET hello
```

Typical responses:
- SET → `+OK`
- GET found → `$5\r\nvalue`
- GET not found → `$-1`
- DEL → `:1` or `:0`
- QUIT → `+BYE`

## Limitations & Security
- Educational/demo code — not production-ready.
- No authentication, no persistence, minimal protocol handling.
- Single-process in-memory store; data lost on exit.

## Contributing & License
- Add issues or pull requests on GitHub.
- Include a LICENSE (MIT recommended) before publishing.