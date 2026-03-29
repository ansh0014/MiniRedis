# MiniRedis - Deployment and DevOps Plan

## 1. Deployment Environments

## Local (Developer)
- Tooling: Docker Compose
- Purpose: feature development and integration validation
- Characteristics:
  - direct host port mapping
  - local `.env`-driven config
  - fast iteration

## Staging
- Tooling: Kubernetes namespace (`miniredis-staging`)
- Purpose: pre-production integration and release validation
- Characteristics:
  - production-like routing and policies
  - synthetic load and smoke tests
  - rollback rehearsal

## Production
- Tooling: Kubernetes namespace (`miniredis-prod`)
- Purpose: live workloads
- Characteristics:
  - managed TLS, secret management, observability stack
  - autoscaling and resilience controls
  - progressive delivery

---

## 2. Container and Build Strategy
- One service per image.
- Multi-stage Docker builds.
- Immutable tags:
  - `service:<git-sha>`
  - optional semver tags for releases.
- Registry push from CI after tests pass.
- Runtime images kept minimal for attack-surface reduction.

---

## 3. Kubernetes Design

## 3.1 Core Workloads
- `Deployment`:
  - frontend
  - api-gateway
  - backend
  - node-manager
  - monitoring-service
  - auth-service
- `StatefulSet`:
  - postgres-main (if self-hosted)
  - postgres-auth (if self-hosted)

## 3.2 Core Objects
- `Service` (ClusterIP) for internal communication.
- `Ingress` for external access.
- `ConfigMap` for non-sensitive config.
- `Secret` for credentials/tokens/keys.
- `PVC` for stateful storage.

---

## 4. Port and Service Mapping for K8s

| Logical Service | App Port | K8s Service Port | Public Exposure |
|---|---:|---:|---|
| frontend | 5173 | 80/443 via ingress | public |
| api-gateway | 8080 | 8080 | public/internal |
| backend | 5500 | 5500 | internal |
| node-manager | 7000 | 7000 | internal |
| monitoring-service | 9000 | 9000 | internal via gateway or restricted public |
| auth-service | 8000 | 8000 | internal via gateway |
| postgres-main | 5432 | 5432 | internal only |
| postgres-auth | 5432 | 5432 | internal only |

Tenant node port pool:
- controlled by env (`REDIS_PORT_START`, `REDIS_PORT_END`)
- verify no conflict with node/host networking strategy.

---

## 5. Reliability Standards

Each service should have:
- readiness probe,
- liveness probe,
- CPU/memory requests and limits,
- rolling update strategy.

Critical services should also have:
- PodDisruptionBudget,
- anti-affinity rules,
- HPA (api-gateway, backend, monitoring).

---

## 6. Security Controls
- Namespace segmentation by environment.
- NetworkPolicy default deny + explicit allow.
- TLS managed with cert-manager.
- Secrets from secret manager (or sealed secrets).
- Non-root container execution where possible.
- CI image and dependency scanning.

---

## 7. Observability Stack
- Metrics: Prometheus
- Dashboards: Grafana
- Logs: Loki
- Traces: OpenTelemetry collector (planned)

Minimum dashboards:
- gateway latency/error rate
- backend request/error trends
- node-manager active node count + memory
- monitoring freshness and scrape health
- database health and connection saturation

---

## 8. CI/CD Pipeline (Target)

## CI
1. Lint and static checks.
2. Unit tests.
3. Integration tests (compose/k8s ephemeral).
4. Build and scan images.
5. Push versioned images.

## CD (GitOps)
1. Update Helm values/image tags.
2. Argo CD sync to staging.
3. Smoke test gate.
4. Promote to production.
5. Monitor rollback triggers.

---

## 9. Rollout and Rollback
- Default: rolling update.
- Sensitive services: canary rollout.
- Auto rollback triggers:
  - readiness failures,
  - elevated 5xx rates,
  - health-check failure budget breach.

---

## 10. Backup/Recovery Plan
- Daily full backups for DB.
- Periodic restore test in staging.
- RPO/RTO documented for production.
- Versioned migration policy for schema changes.

---

## 11. Deployment Milestones
1. Helm charts for all current services.
2. Staging namespace with ingress + TLS.
3. Observability baseline in cluster.
4. GitOps promotion workflow.
5. Production hardening and runbooks.