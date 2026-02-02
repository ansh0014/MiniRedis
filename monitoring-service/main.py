import asyncio
import os
import time
from typing import Dict, List
from contextlib import asynccontextmanager

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
import httpx
import redis.asyncio as redis
from dotenv import load_dotenv

load_dotenv()



NODE_MANAGER_URL = os.getenv("NODE_MANAGER_URL", "http://localhost:7000")
REDIS_URL = os.getenv("REDIS_URL")

POLL_INTERVAL = int(os.getenv("METRICS_POLL_INTERVAL", "1"))
MEMORY_LIMIT_BYTES = 40 * 1024 * 1024  

redis_client: redis.Redis | None = None
http_client: httpx.AsyncClient | None = None
collector_task: asyncio.Task | None = None



@asynccontextmanager
async def lifespan(app: FastAPI):
    global redis_client, http_client, collector_task

   
    redis_client = redis.from_url(
        REDIS_URL,
        decode_responses=True
    )

    http_client = httpx.AsyncClient(
        timeout=httpx.Timeout(
            connect=2.0,
            read=2.0,
            write=2.0,
            pool=2.0
        )
    )

    collector_task = asyncio.create_task(metrics_collector())

    yield

    if collector_task:
        collector_task.cancel()

    if http_client:
        await http_client.aclose()

    if redis_client:
        await redis_client.close()




app = FastAPI(
    title="MiniRedis Monitoring Service (Redis-backed)",
    lifespan=lifespan
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:5173"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)



async def fetch_node_manager_data() -> Dict[str, Dict]:
    try:
        response = await asyncio.wait_for(
            http_client.get(f"{NODE_MANAGER_URL}/node/list"),
            timeout=3
        )

        if response.status_code != 200:
            return {}

        data = response.json()
        return {
            n["tenant_id"]: n
            for n in data
            if isinstance(n, dict) and "tenant_id" in n
        }

    except Exception as e:
        print("[NODE MANAGER ERROR]", e)
        return {}



async def metrics_collector():
    try:
        await asyncio.sleep(2)

        while True:
            print("[COLLECTOR] tick")

            node_data = await fetch_node_manager_data()
            now = int(time.time())

            for tenant_id, node in node_data.items():
                memory_used = int(node.get("memory_used", 0))

                await redis_client.hset(
                    f"monitor:node:{tenant_id}",
                    mapping={
                        "tenant_id": tenant_id,
                        "status": node.get("status", "stopped"),
                        "memory_bytes": memory_used,
                        "memory_usage_percent": (
                            (memory_used / MEMORY_LIMIT_BYTES) * 100
                            if memory_used else 0
                        ),
                        "key_count": int(node.get("key_count", 0)),
                        "connected_clients": 1,
                        "last_updated": now
                    }
                )

        
                await redis_client.expire(f"monitor:node:{tenant_id}", 60)
                await redis_client.sadd("monitor:nodes", tenant_id)

            await asyncio.sleep(POLL_INTERVAL)

    except asyncio.CancelledError:
        print("[COLLECTOR] stopped")


async def get_all_metrics() -> List[Dict]:
    tenant_ids = await redis_client.smembers("monitor:nodes")
    result = []

    for tenant_id in tenant_ids:
        data = await redis_client.hgetall(f"monitor:node:{tenant_id}")
        if not data:
            continue

        mem = int(data.get("memory_bytes", 0))
        data["memory_used_mb"] = mem / (1024 * 1024)
        data["memory_limit_mb"] = 40
        data["memory_used_human"] = f"{data['memory_used_mb']:.2f} MB"

        result.append(data)

    return result


async def get_node_info(tenant_id: str) -> Dict | None:
    data = await redis_client.hgetall(f"monitor:node:{tenant_id}")
    if not data:
        return None

    mem = int(data.get("memory_bytes", 0))
    data["connected"] = data.get("status") == "running"
    data["used_memory_human"] = f"{mem / (1024 * 1024):.2f} MB"

    return data




@app.get("/monitoring/nodes")
async def monitoring_nodes():
    return await get_all_metrics()


@app.get("/monitoring/redis/{tenant_id}")
async def monitoring_redis(tenant_id: str):
    data = await get_node_info(tenant_id)
    if not data:
        return {"error": "Tenant not found"}
    return data


@app.get("/health")
async def health():
    return {"status": "ok", "backend": "redis"}

