# MiniRedis Kubernetes Execution Plan (Minikube)

## Goal
Deploy the current MiniRedis stack from Docker Compose to Kubernetes in a controlled sequence with validation at each step.

---

## Phase 0 - Preconditions

- [ ] Docker Desktop running
- [ ] Minikube installed
- [ ] kubectl installed
- [ ] Helm installed (recommended)
- [ ] Repository cloned at `d:\MiniRedis\MiniRedis`
- [ ] All services run successfully once in Docker Compose baseline

---

## Phase 1 - Local Cluster Setup

- [ ] Start Minikube cluster
- [ ] Enable ingress addon
- [ ] Enable metrics-server addon
- [ ] Confirm cluster health
- [ ] Create namespace: `miniredis-dev`
- [ ] Set current context namespace to `miniredis-dev`

Validation:
- [ ] `kubectl get nodes` healthy
- [ ] `kubectl get pods -A` no critical failures

---

## Phase 2 - Image Strategy

Choose one mode:

### Option A (recommended for Minikube learning)
- [ ] Build images inside Minikube Docker daemon
- [ ] Use local image tags for all services

### Option B
- [ ] Push images to registry
- [ ] Reference registry image tags in manifests

Images required:
- [ ] `frontend`
- [ ] `api-gateway`
- [ ] `backend`
- [ ] `node-manager`
- [ ] `monitoring-service`
- [ ] `auth-service`

Validation:
- [ ] All required images visible to cluster runtime

---

## Phase 3 - Configuration and Secrets

  - [ ] Create ConfigMap for shared non-secret env
  - [ ] Create ConfigMap per service if needed
  - [ ] Create Secrets for:
  - [ ] `POSTGRES_MAIN_PASSWORD`
  - [ ] `POSTGRES_AUTH_PASSWORD`
  - [ ] Firebase credentials / service account
  - [ ] Any API keys or signing secrets
  - [ ] Verify env key naming consistency with app code

Validation:
- [ ] Secret and ConfigMap objects present
- [ ] No missing env references in pod specs

---

## Phase 4 - Database Layer (First Deploy)

- [ ] Deploy `postgres-main` StatefulSet + Service + PVC
- [ ] Deploy `postgres-auth` StatefulSet + Service + PVC
- [ ] Apply init SQL/migrations
- [ ] Add readiness/liveness probes
- [ ] Add storage class and retention policy decision

Validation:
- [ ] Both DB pods Ready
- [ ] DB services reachable by DNS
- [ ] Tables created successfully

---

## Phase 5 - Core Backend Services

Deploy in strict order:

1. [ ] `auth-service`
2. [ ] `backend`
3. [ ] `node-manager`
4. [ ] `monitoring-service`
5. [ ] `api-gateway`
6. [ ] `frontend`

For each service:
- [ ] Deployment
- [ ] ClusterIP Service
- [ ] Readiness probe
- [ ] Liveness probe
- [ ] Resource requests/limits
- [ ] Correct env and upstream URLs

Validation after each deploy:
- [ ] Pod Ready
- [ ] No CrashLoopBackOff
- [ ] Health endpoint responds
- [ ] Service DNS resolution works

---

## Phase 6 - Ingress and Access

- [ ] Deploy ingress resource
- [ ] Route frontend + API paths
- [ ] Configure hostnames for local testing
- [ ] Update local hosts file if required
- [ ] Verify CORS and cookie/auth flow end-to-end

Validation:
- [ ] Login works
- [ ] Dashboard loads
- [ ] Tenant create/start works
- [ ] Monitoring shows running nodes
- [ ] Redis CLI executes commands

---

## Phase 7 - Functional Test Checklist

- [ ] Auth login and token refresh
- [ ] Create tenant instance
- [ ] Start node
- [ ] List node in dashboard
- [ ] Node visible in monitoring
- [ ] Run command in Redis CLI
- [ ] Stop node and verify status update
- [ ] Restart and verify persistence expectations

---

## Phase 8 - Observability Baseline

- [ ] Standardize service logs
- [ ] Add metrics scraping targets
- [ ] Deploy Prometheus/Grafana (optional in dev, recommended in staging)
- [ ] Add dashboard for:
  - [ ] gateway error rate
  - [ ] node-manager active nodes
  - [ ] monitoring freshness
  - [ ] DB connection health

Validation:
- [ ] Metrics visible
- [ ] Logs searchable per service

---

## Phase 9 - Reliability Hardening

- [ ] Add PodDisruptionBudget for critical services
- [ ] Add anti-affinity for gateway/backend (optional in minikube)
- [ ] Add HPA for stateless services (later after metrics baseline)
- [ ] Add retry/timeouts in inter-service calls
- [ ] Add graceful shutdown behavior

Validation:
- [ ] Controlled rollout without downtime spikes
- [ ] Pod restarts recover correctly

---

## Phase 10 - Data and Backup Plan

Current recommendation:
- [ ] Keep DB in-cluster for learning
- [ ] Add backup CronJob (pg_dump)
- [ ] Store backups on host-mounted path or object storage
- [ ] Test restore procedure once per sprint

Future production recommendation:
- [ ] Move to managed Postgres
- [ ] Keep app in k8s, DB external

Validation:
- [ ] Backup artifacts generated
- [ ] Restore test completed

---

## Phase 11 - Release Workflow

- [ ] Define version tags per service
- [ ] Add CI pipeline: lint, test, build, scan, push
- [ ] Add CD flow (manual first, GitOps later)
- [ ] Maintain changelog and rollback checklist

---

## Day-by-Day Execution (Suggested)

### Day 1
- [ ] Phase 0, 1, 2 complete
- [ ] Namespace + images ready

### Day 2
- [ ] Phase 3 and 4 complete
- [ ] Both Postgres stable

### Day 3
- [ ] Phase 5 complete
- [ ] Core services healthy

### Day 4
- [ ] Phase 6 and 7 complete
- [ ] End-to-end flow working

### Day 5
- [ ] Phase 8, 9 baseline
- [ ] Basic reliability checks

### Day 6
- [ ] Phase 10 backup/restore dry run

### Day 7
- [ ] Phase 11 release process baseline

---

## Exit Criteria (Project Milestone)

- [ ] Full stack running in Minikube
- [ ] Core user journey works end-to-end
- [ ] No critical pod crash loops
- [ ] DB persistence verified
- [ ] Monitoring reflects live node-manager data
- [ ] Documented rollback and backup procedures available