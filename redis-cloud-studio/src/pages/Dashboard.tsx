import { useState, useEffect } from "react";
import { useNavigate } from "react-router-dom";
import Navbar from "@/components/Navbar";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import Terminal from "@/components/Terminal";
import { Play, Square, Plus, Database, Activity, Server, AlertCircle, Search } from "lucide-react";
import { api, type RedisNode, type User } from "@/lib/api";
import { toast } from "sonner";

export default function Dashboard() {
  const navigate = useNavigate();
  const [user, setUser] = useState<User | null>(null);
  const [nodes, setNodes] = useState<RedisNode[]>([]);
  const [loading, setLoading] = useState(true);
  const [creatingInstance, setCreatingInstance] = useState(false);
  const [newInstanceName, setNewInstanceName] = useState("");
  const [searchTerm, setSearchTerm] = useState("");

  useEffect(() => { loadUserAndData(); }, []);
  useEffect(() => {
    const id = setInterval(() => { loadNodes(); }, 2000);
    return () => clearInterval(id);
  }, []);

  const loadUserAndData = async () => {
    try {
      const userData = await api.me();
      setUser(userData);
      await loadNodes();
    } catch {
      navigate("/login");
    } finally {
      setLoading(false);
    }
  };

  const loadNodes = async () => {
    try {
      const list = await api.listNodes();
      setNodes(Array.isArray(list) ? list : []);
    } catch {
      setNodes([]);
    }
  };

  const handleCreateInstance = async () => {
    if (!newInstanceName.trim()) { toast.error("Instance name required"); return; }
    setCreatingInstance(true);
    try {
      const tenant = await api.createTenant(newInstanceName, 40);
      const tenantId = String(tenant?.id ?? tenant?.tenant_id ?? tenant?.tenant?.id ?? "");
      const tenantPort = Number(tenant?.port ?? tenant?.node_port ?? tenant?.tenant?.port ?? tenant?.tenant?.node_port ?? 6380);
      if (tenantId && tenantPort) await api.startNode(tenantId, tenantPort);
      toast.success(`Instance "${newInstanceName}" deployed on :${tenantPort}`);
      setNewInstanceName("");
      await loadNodes();
    } catch (error: any) {
      toast.error(error.message || "Failed to create instance");
    } finally {
      setCreatingInstance(false);
    }
  };

  const handleStartNode = async (tenantId: string) => {
    try {
      const node = nodes.find(n => n.tenant_id === tenantId);
      if (!node) throw new Error("Node not found");
      await api.startNode(tenantId, node.port);
      toast.success("Node started");
      await loadNodes();
    } catch (error: any) {
      toast.error(error.message || "Failed to start node");
    }
  };

  const handleStopNode = async (tenantId: string) => {
    try {
      await api.stopNode(tenantId);
      toast.success("Node stopped");
      await loadNodes();
    } catch (error: any) {
      toast.error(error.message || "Failed to stop node");
    }
  };

  const filteredNodes = nodes.filter((n) => {
    const q = searchTerm.toLowerCase();
    return (
      String(n?.tenant_id ?? "").toLowerCase().includes(q) ||
      String((n as any)?.name ?? "").toLowerCase().includes(q) ||
      String(n?.status ?? "").toLowerCase().includes(q)
    );
  });

  const runningCount = nodes.filter(n => n.status === "running").length;
  const stoppedCount = nodes.filter(n => n.status === "stopped").length;

  if (loading) {
    return (
      <div className="flex items-center justify-center h-screen hero-bg">
        <div className="text-center space-y-4">
          <div className="w-10 h-10 border border-emerald-500/30 border-t-emerald-400 rounded-full animate-spin mx-auto" />
          <p className="label-tag">INITIALIZING</p>
        </div>
      </div>
    );
  }

  return (
    <div className="min-h-screen page-bg">
      <Navbar />
      <main className="container mx-auto px-6 py-8 space-y-8 max-w-7xl">

        {/* Header */}
        <div className="flex items-start justify-between animate-fade-up">
          <div className="space-y-1">
            <div className="label-tag">Control Plane</div>
            <h1 className="text-3xl font-extrabold text-white/90 tracking-tight">Dashboard</h1>
            {user && (
              <p className="text-sm text-white/30 mono">
                Session: <span className="text-emerald-400/60">{user.email}</span>
              </p>
            )}
          </div>
          <div className="flex items-center gap-2 px-3 py-1.5 rounded bg-emerald-500/5 border border-emerald-500/15">
            <div className="w-2 h-2 rounded-full bg-emerald-400 animate-pulse" />
            <span className="mono text-xs text-emerald-400/70 tracking-wider">LIVE</span>
          </div>
        </div>

        {/* Stat Cards */}
        <div className="grid gap-4 md:grid-cols-3 animate-fade-up animate-delay-100">
          {[
            { label: "Total Instances", value: nodes.length, icon: <Database className="h-4 w-4" />, color: "emerald" },
            { label: "Running", value: runningCount, icon: <Activity className="h-4 w-4" />, color: "green" },
            { label: "Stopped", value: stoppedCount, icon: <Server className="h-4 w-4" />, color: "red" },
          ].map((stat) => (
            <div key={stat.label} className="stat-card rounded-xl p-6">
              <div className="flex items-center justify-between mb-4">
                <span className="label-tag">{stat.label}</span>
                <div className={`p-2 rounded-lg ${stat.color === 'emerald' ? 'bg-emerald-500/10 text-emerald-400' : stat.color === 'green' ? 'bg-green-500/10 text-green-400' : 'bg-red-500/10 text-red-400'}`}>
                  {stat.icon}
                </div>
              </div>
              <div className="text-4xl font-extrabold text-white/90 mono">{stat.value}</div>
            </div>
          ))}
        </div>

        {/* Create Instance */}
        <div className="glass-card rounded-xl p-6 space-y-4 animate-fade-up animate-delay-200">
          <div className="flex items-center gap-3">
            <div className="w-px h-4 bg-emerald-500" />
            <span className="label-tag">Deploy New Instance</span>
          </div>
          <div className="flex gap-3">
            <Input
              id="instance-name-input"
              placeholder="instance-name (e.g. my-app-cache)"
              value={newInstanceName}
              onChange={(e) => setNewInstanceName(e.target.value)}
              disabled={creatingInstance}
              onKeyDown={(e) => { if (e.key === 'Enter') handleCreateInstance(); }}
              className="flex-1 bg-black/40 border-white/8 text-white/80 placeholder:text-white/20 mono text-sm focus:border-emerald-500/40 focus:ring-emerald-500/20"
            />
            <Button
              id="create-instance-btn"
              onClick={handleCreateInstance}
              disabled={creatingInstance || !newInstanceName.trim()}
              className="bg-emerald-500/15 border border-emerald-500/30 text-emerald-400 hover:bg-emerald-500/25 hover:border-emerald-500/50 transition-all mono text-xs tracking-wider gap-2 px-5"
            >
              <Plus className="h-3.5 w-3.5" />
              {creatingInstance ? "DEPLOYING..." : "DEPLOY"}
            </Button>
          </div>
        </div>

        {/* Instances List */}
        <div className="space-y-4 animate-fade-up animate-delay-300">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-3">
              <div className="w-px h-4 bg-emerald-500" />
              <span className="label-tag">Active Instances</span>
            </div>
            {nodes.length > 0 && (
              <div className="relative">
                <Search className="absolute left-3 top-1/2 -translate-y-1/2 h-3.5 w-3.5 text-white/25" />
                <Input
                  placeholder="Search..."
                  value={searchTerm}
                  onChange={(e) => setSearchTerm(e.target.value)}
                  className="w-48 h-9 pl-8 bg-black/40 border-white/8 text-white/70 placeholder:text-white/20 mono text-xs focus:border-emerald-500/30"
                />
              </div>
            )}
          </div>

          {nodes.length === 0 ? (
            <div className="glass-card rounded-xl p-16 text-center space-y-4">
              <AlertCircle className="w-12 h-12 text-white/15 mx-auto" />
              <p className="text-sm text-white/30 mono">No instances deployed. Create your first one above.</p>
            </div>
          ) : (
            <div className="space-y-2">
              {filteredNodes.map((node) => {
                const isRunning = node.status === "running";
                return (
                  <div
                    key={node.tenant_id}
                    className={`node-card ${isRunning ? 'running' : 'stopped'} rounded-xl px-6 py-5`}
                  >
                    <div className="flex items-center justify-between">
                      <div className="flex items-center gap-5">
                        <div className={`p-2.5 rounded-lg ${isRunning ? 'bg-emerald-500/10' : 'bg-white/5'}`}>
                          <Database className={`h-5 w-5 ${isRunning ? 'text-emerald-400' : 'text-white/25'}`} />
                        </div>
                        <div>
                          <div className="flex items-center gap-3">
                            <h3 className="font-bold text-white/80 text-sm">{node.tenant_id}</h3>
                            <div className={`inline-flex items-center gap-1.5 px-2 py-0.5 rounded-full text-xs mono ${isRunning ? 'bg-emerald-500/10 text-emerald-400 border border-emerald-500/20' : 'bg-white/5 text-white/30 border border-white/8'}`}>
                              <div className={`w-1.5 h-1.5 rounded-full ${isRunning ? 'bg-emerald-400 animate-pulse' : 'bg-white/25'}`} />
                              {node.status.toUpperCase()}
                            </div>
                          </div>
                          <p className="text-xs text-white/30 mono mt-1">
                            PORT: {node.port}
                            {node.created_at && <> &bull; CREATED: {new Date(node.created_at).toLocaleDateString()}</>}
                          </p>
                        </div>
                      </div>

                      <div>
                        {isRunning ? (
                          <button
                            id={`stop-${node.tenant_id}`}
                            onClick={() => handleStopNode(node.tenant_id)}
                            className="flex items-center gap-2 px-4 py-2 rounded-lg border border-red-500/25 bg-red-500/8 text-red-400/70 hover:bg-red-500/15 hover:border-red-500/40 hover:text-red-400 transition-all mono text-xs tracking-wider"
                          >
                            <Square className="h-3.5 w-3.5" />
                            STOP
                          </button>
                        ) : (
                          <button
                            id={`start-${node.tenant_id}`}
                            onClick={() => handleStartNode(node.tenant_id)}
                            className="flex items-center gap-2 px-4 py-2 rounded-lg border border-emerald-500/25 bg-emerald-500/8 text-emerald-400/70 hover:bg-emerald-500/15 hover:border-emerald-500/40 hover:text-emerald-400 transition-all mono text-xs tracking-wider"
                          >
                            <Play className="h-3.5 w-3.5" />
                            START
                          </button>
                        )}
                      </div>
                    </div>
                  </div>
                );
              })}
            </div>
          )}
        </div>

        {/* Terminal */}
        {nodes.length > 0 && (
          <div className="space-y-4 animate-fade-up animate-delay-400">
            <div className="flex items-center gap-3">
              <div className="w-px h-4 bg-emerald-500" />
              <span className="label-tag">Redis CLI</span>
            </div>
            <div className="terminal-window rounded-xl overflow-hidden">
              <div className="terminal-header">
                <div className="terminal-dot bg-red-500/90" />
                <div className="terminal-dot bg-amber-500/90" />
                <div className="terminal-dot bg-emerald-500/90" />
                <span className="ml-2 text-xs text-white/30 mono">redis-cli — interactive</span>
              </div>
              <div className="p-4">
                <Terminal nodes={nodes} />
              </div>
            </div>
          </div>
        )}
      </main>
    </div>
  );
}
