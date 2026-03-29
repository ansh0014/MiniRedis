# MiniRedis - Current Project Documentation

## 1. Project Summary
MiniRedis is a multi-service platform that provides:
- tenant lifecycle management,
- in-memory Redis-like node management,
- command execution from UI,
- monitoring of running nodes,
- authentication and API gateway routing.

It is designed as a cloud-style control plane + in-memory data plane.

---

## 2. Repository Structure (Current)
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

### Service folders
- `redis-cloud-studio/`  
  Frontend (React/Vite) + embedded auth service folder.
- `redis-cloud-studio/auth/`  
  Auth service (Go).
- `api-gateway/`  
  Gateway service (Go) for service aggregation.
- `Backend/`  
  Core metadata and business APIs (C++ Drogon + PostgreSQL).
- `node/`  
  In-memory node manager (C++ Drogon).
- `monitoring-service/`  
  Monitoring API (Python FastAPI).
- `config/`  
  Shared config headers/sources for C++ services.
- `router/`  
  Separate router service (if enabled in deployment topology).

---

## 3. System Architecture

## 3.1 Request Flow
1. Browser (`frontend`) calls `api-gateway`.
2. `api-gateway` routes requests to:
   - `auth-service` for login/session,
   - `backend` for tenant metadata,
   - `node-manager` for node operations,
   - `monitoring-service` for metrics APIs.
3. `node-manager` is the runtime source for live node status and memory/key metrics.
4. PostgreSQL stores persistent metadata and auth data.

## 3.2 Data Ownership
- **Live runtime metrics**: `node-manager` (in-memory)
- **Persistent metadata**: `postgres-main`
- **Auth/session-related DB data**: `postgres-auth`
- **Historical metrics (optional)**: `node_metrics` table if populated

---

## 4. Ports and Network Matrix

All services are attached to `miniredis-network` (bridge in Docker Compose).

| Service | Container Port | Host Port | Source |
|---|---:|---:|---|
| frontend | 5173 | 5173 | fixed in compose |
| api-gateway | 8080 | `${GATEWAY_PORT}` (typically 8080) | env-driven |
| backend | 5500 | `${BACKEND_PORT}` (typically 5500) | env-driven |
| node-manager | 7000 | `${NODE_MANAGER_PORT}` (typically 7000) | env-driven |
| monitoring-service | 9000 | `${MONITORING_SERVICE_PORT}` (typically 9000) | env-driven |
| auth-service | 8000 | `${AUTH_SERVICE_PORT}` (typically 8000) | env-driven |
| postgres-main | 5432 | 5432 | fixed in compose |
| postgres-auth | 5432 | 5433 | fixed in compose |

### Tenant node runtime port range
Tenant nodes are allocated from:
- `REDIS_PORT_START`
- `REDIS_PORT_END`

Available tenant ports count formula:
```text
available_ports = REDIS_PORT_END - REDIS_PORT_START + 1
```

Example:
- if `REDIS_PORT_START=6000` and `REDIS_PORT_END=6999`
- total available tenant ports = `1000`

This is your max node-port capacity before port exhaustion (subject to runtime constraints).

---

## 5. Service Responsibilities

## frontend (`redis-cloud-studio/src`)
- Pages: Dashboard, Monitoring, RedisCLI, Login.
- Creates tenant instances and controls start/stop.
- Displays nodes and metrics.
- Requires robust response normalization for node lists.

## api-gateway
- Single entry point for browser traffic.
- Proxies and normalizes API access to internal services.

## auth-service
- Firebase-backed login/refresh workflow.
- Session and token management.

## backend
- Tenant metadata and API key operations.
- Persistent storage in `postgres-main`.

## node-manager
- Runtime manager for in-memory nodes.
- Node start/stop/list/execute.
- Source of live node status and usage values.

## monitoring-service
- Pulls node list/metrics from node-manager.
- Exposes monitoring endpoints for frontend monitoring page.

---

## 6. Current Functional Scope
- Login/auth flow
- Create tenant instance from dashboard
- Start/stop node lifecycle
- Execute Redis-style commands in RedisCLI
- Monitoring page for active instances
- Docker Compose local orchestration

---

## 7. Known Operational Considerations
1. Node-list API shape mismatch can break Dashboard (`nodes.filter` error).
2. Monitoring service import/dependency issues can crash Uvicorn startup.
3. `created_at` may be missing for runtime-only node payloads; UI should fallback.
4. Runtime metrics should come from node-manager for real-time correctness.

---

## 8. Capacity and Limits (Current)
- Tenant runtime ports limited by `REDIS_PORT_START..REDIS_PORT_END`.
- Per-node memory limit currently configured at creation (example: `40 MB`).
- Total active nodes bounded by:
  - available port range,
  - host memory/CPU,
  - node-manager process constraints.

---

## 9. Validation Checklist
- [ ] All services healthy in `docker compose ps`
- [ ] Dashboard shows created node in "Your Instances"
- [ ] Monitoring reflects running node count > 0
- [ ] RedisCLI executes command on selected tenant node
- [ ] Gateway health endpoint responds
- [ ] Auth refresh flow works after token expiry