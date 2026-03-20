import asyncio
import logging
from contextlib import asynccontextmanager
from datetime import datetime
import os

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
import psycopg2
import requests

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

db_conn = None
NODE_MANAGER_URL = os.getenv("NODE_MANAGER_URL", "http://node-manager:7000")

@asynccontextmanager
async def lifespan(app: FastAPI):
    global db_conn
    try:
        db_conn = psycopg2.connect(
            host=os.getenv("DB_HOST", "postgres-main"),
            port=int(os.getenv("DB_PORT", "5432")),
            database=os.getenv("DB_NAME", "miniredis"),
            user=os.getenv("DB_USER", "postgres"),
            password=os.getenv("DB_PASSWORD"),
        )
        logger.info("PostgreSQL connected")
    except Exception as e:
        db_conn = None
        logger.error(f"PostgreSQL connection failed: {e}")
    yield
    if db_conn and not db_conn.closed:
        db_conn.close()

app = FastAPI(title="Monitoring Service", lifespan=lifespan)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:5173", "http://localhost:80", "http://localhost:8080"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/health")
async def health():
    db_status = "connected" if db_conn and not db_conn.closed else "disconnected"
    return {
        "status": "healthy",
        "service": "monitoring",
        "database": db_status,
        "timestamp": datetime.utcnow().isoformat(),
    }

@app.get("/monitoring/nodes")
async def get_all_nodes():
    try:
        r = requests.get(f"{NODE_MANAGER_URL}/node/list", timeout=5)
        r.raise_for_status()
        data = r.json()

        nodes = data if isinstance(data, list) else data.get("nodes", [])
        if not isinstance(nodes, list):
            return []

        results = []
        for node in nodes:
            memory_bytes = int(node.get("memory_bytes", 0) or 0)
            memory_limit_mb = int(node.get("memory_limit_mb", 40) or 40)
            memory_used_mb = round(memory_bytes / 1048576, 2)
            memory_usage_percent = round((memory_bytes / (memory_limit_mb * 1048576)) * 100, 2) if memory_limit_mb > 0 else 0.0

            if memory_bytes < 1024:
                memory_human = f"{memory_bytes} B"
            elif memory_bytes < 1048576:
                memory_human = f"{memory_bytes / 1024:.2f} KB"
            elif memory_bytes < 1073741824:
                memory_human = f"{memory_bytes / 1048576:.2f} MB"
            else:
                memory_human = f"{memory_bytes / 1073741824:.2f} GB"

            port = int(node.get("port", 0) or 0)
            results.append({
                "tenant_id": node.get("tenant_id"),
                "name": node.get("tenant_id"),
                "port": port,
                "status": node.get("status", "running"),
                "key_count": int(node.get("key_count", 0) or 0),
                "memory_used_bytes": memory_bytes,
                "memory_used_mb": memory_used_mb,
                "memory_limit_mb": memory_limit_mb,
                "memory_usage_percent": memory_usage_percent,
                "memory_used_human": memory_human,
                "connected_clients": int(node.get("connected_clients", 1) or 1),
                "redis_cli_command": f"redis-cli -h localhost -p {port}",
            })

        return results
    except Exception as e:
        logger.error(f"Error fetching nodes from node-manager: {e}")
        return []

@app.get("/monitoring/stats/summary")
async def summary():
    nodes = await get_all_nodes()
    running = [n for n in nodes if str(n.get("status", "")).lower() == "running"]
    total_keys = sum(int(n.get("key_count", 0) or 0) for n in nodes)
    total_memory_bytes = sum(int(n.get("memory_used_bytes", 0) or 0) for n in nodes)

    return {
        "total_nodes": len(nodes),
        "active_nodes": len(running),
        "total_keys": total_keys,
        "total_memory_mb": round(total_memory_bytes / (1024 * 1024), 2),
    }

@app.get("/monitoring/redis/{tenant_id}")
async def get_redis_info(tenant_id: str):
    if not db_conn or db_conn.closed:
        raise HTTPException(status_code=503, detail="Database unavailable")
    try:
        cursor = db_conn.cursor()
        cursor.execute(
            """
            SELECT
                nm.key_count,
                nm.memory_bytes,
                nm.memory_usage_percent,
                nm.ops_per_sec,
                nm.connected_clients,
                nm.uptime_seconds,
                nm.status,
                nm.last_updated,
                t.node_port,
                t.memory_limit_mb
            FROM node_metrics nm
            JOIN tenants t ON nm.tenant_id = t.id
            WHERE t.id = %s::uuid
            """,
            (tenant_id,),
        )
        row = cursor.fetchone()
        cursor.close()

        if not row:
            raise HTTPException(status_code=404, detail="Metrics not found")

        key_count, memory_bytes, mem_percent, ops, clients, uptime, status, last_updated, port, mem_limit = row
        return {
            "tenant_id": tenant_id,
            "port": port,
            "status": status,
            "key_count": int(key_count or 0),
            "memory_bytes": int(memory_bytes or 0),
            "memory_mb": float((memory_bytes or 0) / 1048576),
            "memory_limit_mb": int(mem_limit),
            "memory_usage_percent": float(mem_percent or 0),
            "ops_per_sec": int(ops or 0),
            "connected_clients": int(clients or 0),
            "uptime_seconds": int(uptime or 0),
            "last_updated": last_updated.isoformat() if last_updated else None,
        }
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"Error fetching Redis info: {e}")
        raise HTTPException(status_code=500, detail=str(e))



