import asyncio
import logging
from contextlib import asynccontextmanager
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
import psycopg2
import os
from datetime import datetime

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

db_conn = None

@asynccontextmanager
async def lifespan(app: FastAPI):
    global db_conn
    
    logger.info("🚀 Starting Monitoring Service...")
    
    # Connect to PostgreSQL
    try:
        db_conn = psycopg2.connect(
            host=os.getenv("DB_HOST", "postgres-main"),
            port=int(os.getenv("DB_PORT", "5432")),
            database=os.getenv("DB_NAME", "miniredis"),
            user=os.getenv("DB_USER", "postgres"),
            password=os.getenv("DB_PASSWORD")
        )
        logger.info(" PostgreSQL connected")
    except Exception as e:
        logger.error(f" PostgreSQL connection failed: {e}")
        raise
    
    logger.info(f" Monitoring Service ready on port {os.getenv('MONITORING_SERVICE_PORT', '9000')}")
    
    yield
    
    # Cleanup
    if db_conn:
        db_conn.close()
        logger.info(" PostgreSQL connection closed")

app = FastAPI(title="Monitoring Service", lifespan=lifespan)

# CORS
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
        "timestamp": datetime.utcnow().isoformat()
    }

@app.get("/monitoring/nodes")
async def get_all_nodes():
    """Get all tenant nodes with metrics - returns array directly"""
    if not db_conn or db_conn.closed:
        raise HTTPException(status_code=503, detail="Database unavailable")
    
    try:
        cursor = db_conn.cursor()
        
       
        cursor.execute("""
            SELECT 
                t.id::text as tenant_id,
                t.name,
                t.node_port as port,
                nm.status,
                COALESCE(nm.key_count, 0) as key_count,
                COALESCE(nm.memory_bytes, 0) as memory_used_bytes,
                COALESCE(nm.memory_bytes / 1048576.0, 0) as memory_used_mb,
                t.memory_limit_mb,
                COALESCE(nm.memory_usage_percent, 0) as memory_usage_percent,
                COALESCE(nm.connected_clients, 0) as connected_clients
            FROM tenants t
            LEFT JOIN node_metrics nm ON t.id = nm.tenant_id
            WHERE t.status = 'active'
            ORDER BY t.created_at DESC
        """)
        
        results = []
        for row in cursor.fetchall():
            tenant_id, name, port, status, key_count, memory_bytes, memory_mb, mem_limit, mem_percent, clients = row
            
            # Format memory in human-readable format
            if memory_bytes < 1024:
                memory_human = f"{memory_bytes} B"
            elif memory_bytes < 1048576:
                memory_human = f"{memory_bytes / 1024:.2f} KB"
            elif memory_bytes < 1073741824:
                memory_human = f"{memory_bytes / 1048576:.2f} MB"
            else:
                memory_human = f"{memory_bytes / 1073741824:.2f} GB"
            
            node_data = {
                "tenant_id": tenant_id,
                "name": name or f"Redis-{port}",
                "port": int(port),
                "status": status or "stopped",
                "key_count": int(key_count),
                "memory_used_bytes": int(memory_bytes),
                "memory_used_mb": float(memory_mb),
                "memory_limit_mb": int(mem_limit),
                "memory_usage_percent": float(mem_percent),
                "memory_used_human": memory_human,
                "connected_clients": int(clients),
                "redis_cli_command": f"redis-cli -h localhost -p {port}"
            }
            results.append(node_data)
        
        cursor.close()
        
      
        return results
        
    except Exception as e:
        logger.error(f" Error fetching nodes: {e}")
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/monitoring/redis/{tenant_id}")
async def get_redis_info(tenant_id: str):
    """Get detailed Redis metrics for specific tenant"""
    if not db_conn or db_conn.closed:
        raise HTTPException(status_code=503, detail="Database unavailable")
    
    try:
        cursor = db_conn.cursor()
        cursor.execute("""
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
        """, (tenant_id,))
        
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
            "last_updated": last_updated.isoformat() if last_updated else None
        }
        
    except HTTPException:
        raise
    except Exception as e:
        logger.error(f"Error fetching Redis info: {e}")
        raise HTTPException(status_code=500, detail=str(e))

