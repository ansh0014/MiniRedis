# MiniRedis

MiniRedis is a multi-service, tenant-aware Redis platform with a custom in-memory runtime, web dashboard, CLI, authentication, and monitoring.

---

## Architecture Overview

MiniRedis is split into control-plane and data-plane services:

- **Frontend** (`redis-cloud-studio`) — React + Vite UI
- **API Gateway** (`api-gateway`) — single public API entry
- **Auth Service** (`redis-cloud-studio/auth`) — Firebase-backed auth
- **Backend** (`Backend`) — tenant metadata + API key logic
- **Node Manager** (`node`) — in-memory Redis-like node lifecycle + execution
- **Monitoring Service** (`monitoring-service`) — live node metrics aggregation
- **Postgres Main** — core metadata
- **Postgres Auth** — auth data

### Data ownership

- **Live runtime metrics/state**: `node-manager`
- **Persistent metadata**: `postgres-main`
- **Auth/session data**: `postgres-auth`
- **Historical metrics**: optional (`node_metrics` if enabled/populated)

---

## Repository Structure

```text
MiniRedis/
├── docker-compose.yml
├── Dockerfile.base
├── current.md
├── future.md
├── deployment.md
├── api-gateway/
├── Backend/
├── config/
├── monitoring-service/
├── node/
├── redis-cloud-studio/
└── router/
```

---

## Port Map

| Service | Host Port | Container Port |
|---|---:|---:|
| Frontend | `5173` | `5173` |
| API Gateway | `${GATEWAY_PORT}` (default `8080`) | `8080` |
| Backend | `${BACKEND_PORT}` (default `5500`) | `5500` |
| Node Manager | `${NODE_MANAGER_PORT}` (default `7000`) | `7000` |
| Monitoring Service | `${MONITORING_SERVICE_PORT}` (default `9000`) | `9000` |
| Auth Service | `${AUTH_SERVICE_PORT}` (default `8000`) | `8000` |
| Postgres Main | `5432` | `5432` |
| Postgres Auth | `5433` | `5432` |

### Tenant runtime port capacity

Tenant node ports are allocated from:

- `REDIS_PORT_START`
- `REDIS_PORT_END`

Capacity formula:

```text
available_ports = REDIS_PORT_END - REDIS_PORT_START + 1
```

Example: `6000..6999` gives **1000** available tenant ports.

---

## Prerequisites

- Docker Desktop (Windows)
- Docker Compose v2
- Node.js 18+ (if running frontend locally without container)
- Git

---

## Quick Start

1. Clone and open project root:

```bash
cd d:\MiniRedis\MiniRedis
```

2. Configure environment variables in root `.env`.

3. Build and start all services:

```bash
docker compose up -d --build
```

4. Verify containers:

```bash
docker compose ps
```

5. Open UI:

- Frontend: `http://localhost:5173`
- Gateway health: `http://localhost:8080/health`
- Monitoring health: `http://localhost:9000/health`

---

## Core User Flow

1. Login from frontend (Google auth).
2. Create tenant instance from Dashboard.
3. Start node (if not auto-started by flow).
4. Use Redis CLI page to execute commands.
5. View live node status and metrics on Monitoring.

---

## API Contract Notes (important)

Node-list payloads can vary by service. Frontend/gateway should normalize:

- `[]`
- `{ "nodes": [] }`
- `{ "data": [] }`

This prevents runtime errors like:

- `nodes.filter is not a function`

---

## Monitoring Model

Current intended model:

- Monitoring service fetches live data from `node-manager` (`/node/list`)
- Monitoring UI renders active nodes from monitoring API
- Postgres metrics table is optional for historical storage, not mandatory for live status

---

## Troubleshooting

### 1) Dashboard error: `nodes.filter is not a function`
Cause: non-array payload returned for node list.

Fix:
- normalize response in frontend `api.ts`
- ensure monitoring/node-manager returns array-compatible shape

### 2) Monitoring container fails at startup (`uvicorn import` stack)
Cause: bad app import path or missing dependencies.

Fix:
- verify monitoring Dockerfile CMD uses `uvicorn main:app ...`
- ensure `r.txt` includes required packages (`fastapi`, `uvicorn`, `requests`, optional `psycopg2-binary`)

### 3) Backend Docker build fails on CMake file
Cause: path/case mismatch for `CMakeLists.txt`.

Fix:
- ensure Dockerfile copies `Backend/CmakeLists.txt` to `/app/CMakeLists.txt` correctly
- check build context in compose is project root (`context: .`)

### 4) Monitoring shows 0 active despite created instance
Cause: monitoring reads DB-only metrics while runtime is in memory.

Fix:
- monitoring must read from node-manager live endpoint

---

## Documentation Index

- [`current.md`](./current.md) — current architecture, ports, capacity, runtime model
- [`future.md`](./future.md) — roadmap and planned services
- [`deployment.md`](./deployment.md) — Kubernetes/DevOps strategy

---

## Planned Services

- Pub/Sub service
- Backup service
- Scheduler service
- Alert service
- Audit service

---

## License

This project is licensed under the terms in [`LICENSE`](./LICENSE).