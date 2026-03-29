# MiniRedis - Future Architecture and Feature Plan

## 1. Product Direction
Evolve MiniRedis into a production-grade, multi-tenant, in-memory platform with:
- reliable runtime node lifecycle,
- historical observability,
- event-driven extensions,
- backup and compliance capabilities.

---

## 2. Planned New Services

## 2.1 Pub/Sub Service
Purpose:
- tenant-scoped publish/subscribe channels.

Capabilities:
- publish API,
- subscribe/consumer API,
- replay policy by tenant level (optional),
- delivery metrics.

Dependencies:
- gateway integration,
- auth context propagation,
- optional NATS/Kafka backend.

## 2.2 Backup Service
Purpose:
- scheduled backup and restore for metadata/state snapshots.

Capabilities:
- scheduled backup jobs,
- checksum verification,
- restore workflows,
- backup catalog.

Dependencies:
- postgres access,
- object storage target.

## 2.3 Scheduler Service
Purpose:
- periodic background tasks.

Capabilities:
- stale-node cleanup,
- metric rollups,
- quota and maintenance jobs.

## 2.4 Alert Service
Purpose:
- operational alerts from monitoring signals.

Capabilities:
- threshold policies,
- rule engine,
- Slack/email/webhook notifications.

## 2.5 Audit Service
Purpose:
- immutable action log for security/compliance.

Capabilities:
- record user actions,
- record privileged operations,
- searchable audit timeline.

---

## 3. Future System Design

## 3.1 Control Plane
- api-gateway + backend + auth + audit + scheduler.
- Handles identity, policy, metadata, and orchestration.

## 3.2 Data Plane
- node-manager and runtime tenant nodes.
- Handles in-memory command execution and live state.

## 3.3 Observability Plane
- monitoring-service + metrics store + alert-service.
- Handles real-time plus historical visibility.

## 3.4 Resilience Plane
- backup-service + restore workflows + runbooks.

---

## 4. Feature Backlog (Priority)

## P0 (stability)
- contract versioning and schema enforcement.
- end-to-end integration test suite.
- strict health/readiness for every service.

## P1 (platform)
- pub/sub service baseline.
- backup/restore baseline.
- scheduler jobs baseline.

## P2 (operations)
- alert routing and escalation.
- audit indexing and retention policy.
- usage metering and tenant billing hooks.

## P3 (scale)
- sharding strategy for node runtime.
- replication/failover model.
- policy-driven tenant QoS classes.

---

## 5. Future Port Planning
Current fixed service ports:
- gateway 8080, backend 5500, node-manager 7000, monitoring 9000, auth 8000, frontend 5173.

Future recommended internal ports:
- pubsub-service: 9100
- backup-service: 9200
- scheduler-service: 9300
- alert-service: 9400
- audit-service: 9500

Keep them internal-only behind gateway unless explicitly required.

---

## 6. Non-Functional Targets
- API availability >= 99.9%
- Monitoring freshness delay < 10s
- p95 control-plane latency < 300ms
- automatic rollback on failed deployment health gates

---

## 7. Delivery Plan
- Wave 1: stabilize contracts and tests.
- Wave 2: deploy pub/sub + scheduler skeleton services.
- Wave 3: backup + alert + audit services.
- Wave 4: scale, policy, and production optimization.

## 8. Terminal & External Integration Features

### 8.1 Redis Protocol Compatibility (RESP)

Purpose:
- allow users to connect using standard redis-cli and libraries

Capabilities:
- implement RESP parser
- support core commands (GET, SET, DEL, EXPIRE)
- handle multiple concurrent TCP connections

---

### 8.2 Public Node Access

Purpose:
- allow external access to tenant nodes

Capabilities:
- expose node ports securely
- return connection details to users
- optional domain-based routing

Example:
redis-cli -h your-domain.com -p 6001

---

### 8.3 Authentication Layer for Nodes

Purpose:
- secure tenant access

Capabilities:
- per-node password
- AUTH command support
- token-based validation via gateway

---

### 8.4 SDK Support (Developer Integration)

Purpose:
- allow developers to use MiniRedis in their apps

Capabilities:
- Node.js SDK
- Python SDK
- connection helpers
- environment-based config

---

### 8.5 Hybrid Multi-Tenant Architecture (Next Evolution)

Purpose:
- remove port-based limitation

Design:
- single node-manager port
- internal tenant routing
- in-memory tenant isolation

Benefits:
- infinite tenant scalability
- no port exhaustion
- better performance

---

### 8.6 Sharding & Load Distribution

Purpose:
- scale system horizontally

Capabilities:
- multiple node-managers
- tenant-to-node mapping
- consistent hashing

---

### 8.7 Persistent Storage Support

Purpose:
- avoid data loss

Capabilities:
- snapshot (RDB-like)
- append-only logs (AOF-like)
- restore mechanism

---

### 8.8 Developer CLI Tool

Purpose:
- improve developer experience

Capabilities:
- custom CLI (miniredis-cli)
- connect, list nodes, monitor usage
- execute commands

---

### 8.9 Rate Limiting & Quotas

Purpose:
- prevent abuse

Capabilities:
- per-tenant limits
- request throttling
- usage tracking

---

### 8.10 Observability for Tenants

Purpose:
- give insights to users

Capabilities:
- memory usage per tenant
- command metrics
- latency tracking