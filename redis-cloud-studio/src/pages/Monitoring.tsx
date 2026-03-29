import { useEffect, useState } from "react";
import Navbar from "@/components/Navbar";
import { Database, Zap, Users, HardDrive, RefreshCw, AlertCircle } from "lucide-react";

interface NodeMonitoring {
  tenant_id: string;
  name: string;
  port: number;
  status: string;
  key_count: number;
  memory_used_bytes: number;
  memory_used_mb: number;
  memory_limit_mb: number;
  memory_usage_percent: number;
  memory_used_human: string;
  connected_clients: number;
  redis_cli_command: string;
}

export default function Monitoring() {
  const [nodes, setNodes] = useState<NodeMonitoring[]>([]);
  const [loading, setLoading] = useState(true);
  const [lastUpdated, setLastUpdated] = useState<Date | null>(null);

  useEffect(() => {
    fetchMonitoringData();
    const interval = setInterval(fetchMonitoringData, 5000);
    return () => clearInterval(interval);
  }, []);

  const fetchMonitoringData = async () => {
    try {
      const response = await fetch("http://localhost:9000/monitoring/nodes");
      if (response.ok) {
        const data = await response.json();
        if (Array.isArray(data)) {
          const sanitized = data
            .map((node: any) => ({
              ...node,
              key_count: Number(node.key_count) || 0,
              memory_used_bytes: Number(node.memory_used_bytes) || 0,
              memory_used_mb: Number(node.memory_used_mb) || 0,
              memory_limit_mb: Number(node.memory_limit_mb) || 0,
              memory_usage_percent: Number(node.memory_usage_percent) || 0,
              connected_clients: Number(node.connected_clients) || 0,
              port: Number(node.port) || 0,
            }))
            .filter((n: NodeMonitoring) => n.status === "running");
          setNodes(sanitized);
          setLastUpdated(new Date());
        }
      }
    } catch {
      setNodes([]);
    } finally {
      setLoading(false);
    }
  };

  if (loading) {
    return (
      <div className="min-h-screen page-bg">
        <Navbar />
        <div className="flex items-center justify-center h-[80vh]">
          <div className="text-center space-y-4">
            <div className="w-10 h-10 border border-emerald-500/30 border-t-emerald-400 rounded-full animate-spin mx-auto" />
            <p className="label-tag">FETCHING METRICS</p>
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className="min-h-screen page-bg">
      <Navbar />
      <div className="container mx-auto px-6 py-8 max-w-7xl">

        {/* Header */}
        <div className="flex items-start justify-between mb-10 animate-fade-up">
          <div className="space-y-1">
            <div className="label-tag">Observability</div>
            <h1 className="text-3xl font-extrabold text-white/90 tracking-tight">Monitoring</h1>
            <p className="text-sm text-white/30 mono">
              {nodes.length} active instance{nodes.length !== 1 ? 's' : ''} — auto-refresh every 5s
            </p>
          </div>
          <div className="flex items-center gap-3">
            {lastUpdated && (
              <span className="text-xs text-white/20 mono">
                Updated {lastUpdated.toLocaleTimeString()}
              </span>
            )}
            <div className="flex items-center gap-2 px-3 py-1.5 rounded bg-emerald-500/5 border border-emerald-500/15">
              <RefreshCw className="w-3 h-3 text-emerald-400 animate-spin" style={{ animationDuration: '3s' }} />
              <span className="mono text-xs text-emerald-400/70">LIVE</span>
            </div>
          </div>
        </div>

        {nodes.length === 0 ? (
          <div className="glass-card rounded-xl p-20 text-center space-y-5 animate-fade-up">
            <AlertCircle className="w-14 h-14 mx-auto text-white/10" />
            <div>
              <h3 className="text-lg font-bold text-white/40 mb-2">No Running Instances</h3>
              <p className="text-sm text-white/20 mono">
                Start a Redis instance from the Dashboard to see live metrics
              </p>
            </div>
          </div>
        ) : (
          <div className="space-y-5">
            {nodes.map((node, idx) => {
              const memPct = Math.min(node.memory_usage_percent || 0, 100);
              const memColor = memPct > 80 ? 'red' : memPct > 60 ? 'amber' : 'emerald';

              return (
                <div key={node.tenant_id} className={`glass-card rounded-xl p-6 space-y-6 animate-fade-up`} style={{ animationDelay: `${idx * 0.08}s` }}>

                  {/* Node header */}
                  <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
                    <div className="flex items-center gap-4">
                      <div className="relative">
                        <div className="w-10 h-10 rounded-xl bg-emerald-500/10 border border-emerald-500/25 flex items-center justify-center">
                          <Database className="w-5 h-5 text-emerald-400" />
                        </div>
                        <div className="absolute -top-0.5 -right-0.5 w-2.5 h-2.5 rounded-full bg-emerald-400 animate-pulse border-2 border-black" />
                      </div>
                      <div>
                        <h3 className="font-bold text-white/90 text-base">{node.name || node.tenant_id}</h3>
                        <p className="text-xs text-white/30 mono mt-0.5">
                          ID: {node.tenant_id} &bull; PORT: {node.port}
                        </p>
                      </div>
                    </div>
                    <div className="inline-flex items-center gap-2 px-3 py-1.5 rounded-full bg-emerald-500/10 border border-emerald-500/20 self-start sm:self-auto">
                      <div className="w-1.5 h-1.5 rounded-full bg-emerald-400 animate-pulse" />
                      <span className="text-xs mono text-emerald-400 tracking-wider">{node.status.toUpperCase()}</span>
                    </div>
                  </div>

                  {/* Metric grid */}
                  <div className="grid grid-cols-2 lg:grid-cols-4 gap-4">
                    {/* Memory Used */}
                    <div className="stat-card rounded-lg p-4 space-y-3">
                      <div className="flex items-center justify-between">
                        <span className="label-tag">Memory Used</span>
                        <div className="p-1.5 rounded bg-emerald-500/10">
                          <HardDrive className="w-3.5 h-3.5 text-emerald-400" />
                        </div>
                      </div>
                      <div className="mono text-xl font-bold text-white/90">
                        {(node.memory_used_mb || 0).toFixed(2)}
                        <span className="text-xs text-white/40 ml-1">MB</span>
                      </div>
                      <div className="neon-progress">
                        <div
                          className="neon-progress-fill"
                          style={{
                            width: `${memPct}%`,
                            background: memColor === 'red'
                              ? 'linear-gradient(90deg, #ef4444, #f97316)'
                              : memColor === 'amber'
                              ? 'linear-gradient(90deg, #f59e0b, #eab308)'
                              : 'linear-gradient(90deg, #10f084, #00e5ff)'
                          }}
                        />
                      </div>
                      <p className="text-xs text-white/25 mono">
                        {memPct.toFixed(1)}% of {node.memory_limit_mb || 0} MB
                      </p>
                    </div>

                    {/* Keys */}
                    <div className="stat-card rounded-lg p-4 space-y-3">
                      <div className="flex items-center justify-between">
                        <span className="label-tag">Total Keys</span>
                        <div className="p-1.5 rounded bg-green-500/10">
                          <Database className="w-3.5 h-3.5 text-green-400" />
                        </div>
                      </div>
                      <div className="mono text-xl font-bold text-white/90">
                        {(node.key_count || 0).toLocaleString()}
                      </div>
                      <div className="neon-progress">
                        <div className="neon-progress-fill" style={{ width: `${Math.min((node.key_count / 10000) * 100, 100)}%`, background: 'linear-gradient(90deg, #22c55e, #10b981)' }} />
                      </div>
                      <p className="text-xs text-white/25 mono">Stored items</p>
                    </div>

                    {/* Clients */}
                    <div className="stat-card rounded-lg p-4 space-y-3">
                      <div className="flex items-center justify-between">
                        <span className="label-tag">Clients</span>
                        <div className="p-1.5 rounded bg-cyan-500/10">
                          <Users className="w-3.5 h-3.5 text-cyan-400" />
                        </div>
                      </div>
                      <div className="mono text-xl font-bold text-white/90">
                        {node.connected_clients || 0}
                      </div>
                      <div className="neon-progress">
                        <div className="neon-progress-fill" style={{ width: `${Math.min((node.connected_clients / 100) * 100, 100)}%`, background: 'linear-gradient(90deg, #00e5ff, #06b6d4)' }} />
                      </div>
                      <p className="text-xs text-white/25 mono">Active connections</p>
                    </div>

                    {/* Memory Human */}
                    <div className="stat-card rounded-lg p-4 space-y-3">
                      <div className="flex items-center justify-between">
                        <span className="label-tag">Memory</span>
                        <div className="p-1.5 rounded bg-violet-500/10">
                          <Zap className="w-3.5 h-3.5 text-violet-400" />
                        </div>
                      </div>
                      <div className="mono text-xl font-bold text-white/90">
                        {node.memory_used_human || "0 B"}
                      </div>
                      <div className="neon-progress">
                        <div className="neon-progress-fill" style={{ width: `${memPct}%`, background: 'linear-gradient(90deg, #a78bfa, #818cf8)' }} />
                      </div>
                      <p className="text-xs text-white/25 mono">Human readable</p>
                    </div>
                  </div>

                  {/* CLI Command */}
                  <div className="flex items-center gap-3 p-3 rounded-lg bg-black/40 border border-white/5">
                    <span className="text-xs text-white/25 mono shrink-0">$ connect:</span>
                    <code className="text-xs mono text-emerald-400/70 truncate">
                      {node.redis_cli_command || `redis-cli -h localhost -p ${node.port}`}
                    </code>
                  </div>
                </div>
              );
            })}
          </div>
        )}
      </div>
    </div>
  );
}
