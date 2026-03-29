# MiniRedis – Deployment Solution (Multi-Port Architecture)

## 1. Objective

Deploy current MiniRedis system using multi-port architecture while enabling:
- terminal access via redis-cli
- external project integration
- stable node lifecycle

---

## 2. Architecture (Current Deployment)

Frontend → API Gateway → Node Manager → Tenant Node (Port-based)

Each tenant gets:
- dedicated port
- isolated in-memory instance

---

## 3. Tenant Node Allocation

PORT RANGE:
REDIS_PORT_START=6000
REDIS_PORT_END=6999

MAX NODES:
TOTAL = END - START + 1

Example:
6000–6999 → 1000 nodes

---

## 4. Node Creation Flow

# Step 1: User creates instance (via UI or API)
POST /create-node

# Step 2: Backend allocates port
PORT=$(get_free_port)

# Step 3: Node Manager starts node
start_node --port=$PORT --tenant_id=$TENANT_ID

# Step 4: Response returned
{
  "host": "your-server-ip",
  "port": 6001
}

---

## 5. Terminal Access (CLI)

# User connects using redis-cli
redis-cli -h <HOST> -p <PORT>

# Example
redis-cli -h 127.0.0.1 -p 6001

# Commands
SET key value
GET key

---

## 6. External Project Usage

# Node.js
const client = redis.createClient({
  socket: { host: "HOST", port: PORT }
});

# Python
r = redis.Redis(host="HOST", port=PORT)

---

## 7. Port Management Strategy

# Maintain port pool
free_ports = [6000...6999]
used_ports = {}

# Allocate port
PORT = free_ports.pop()
used_ports.add(PORT)

# Release port (with delay)
sleep 30
free_ports.push(PORT)

---

## 8. Node Shutdown Handling

# On user stop / disconnect
stop_node(PORT)

# Free memory
flush_all()

# Close socket
close(PORT)

# Reuse port later

---

## 9. Required Deployment Config

# Docker Compose
ports:
  - "6000-6999:6000-6999"

# OR dynamic mapping per node

---

## 10. Health & Stability

# Must implement
- health check endpoint
- auto-restart node
- monitoring integration

---

## 11. Limitations

- port exhaustion possible
- TIME_WAIT delay on reuse
- not horizontally scalable

---

## 12. Conclusion

This approach enables:
- fast deployment
- CLI compatibility
- real usage testing

Future upgrade required:
→ single-port multi-tenant system