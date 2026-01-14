import { useEffect, useState } from "react";
import { Card } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";
import Navbar from "@/components/Navbar";
import { Database, Zap, Users, TrendingUp } from "lucide-react";

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

  useEffect(() => {
    fetchMonitoringData();
    const interval = setInterval(fetchMonitoringData, 5000);
    return () => clearInterval(interval);
  }, []);

  const fetchMonitoringData = async () => {
    try {
      const response = await fetch("http://localhost:9000/monitoring/nodes", {
        method: "GET",
      });

      if (response.ok) {
        const data = await response.json();
        
        if (Array.isArray(data)) {
          // Only show nodes with status "running" - strict check
          const runningNodes = data.filter(
            (node: NodeMonitoring) => node.status === "running"
          );
          setNodes(runningNodes);
        }
      }
    } catch (err) {
      setNodes([]);
    } finally {
      setLoading(false);
    }
  };

  const formatBytes = (bytes: number): string => {
    if (bytes === 0) return "0 B";
    const k = 1024;
    const sizes = ["B", "KB", "MB", "GB"];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return Math.round((bytes / Math.pow(k, i)) * 100) / 100 + " " + sizes[i];
  };

  if (loading) {
    return (
      <div className="min-h-screen bg-background">
        <Navbar />
        <div className="flex items-center justify-center h-[80vh]">
          <div className="animate-spin rounded-full h-32 w-32 border-b-2 border-primary"></div>
        </div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-background">
      <Navbar />
      <div className="container mx-auto px-4 sm:px-6 py-8">
        <div className="mb-8">
          <h1 className="text-3xl font-bold mb-2">Monitoring Dashboard</h1>
          <p className="text-foreground/70">
            Real-time metrics for running Redis instances ({nodes.length} active)
          </p>
        </div>

        {nodes.length === 0 ? (
          <Card className="p-12 text-center">
            <Database className="w-16 h-16 mx-auto mb-4 text-muted-foreground" />
            <h3 className="text-xl font-semibold mb-2">No Running Instances</h3>
            <p className="text-foreground/70">
              Start a Redis instance from Dashboard to see monitoring data
            </p>
          </Card>
        ) : (
          <div className="grid gap-6">
            {nodes.map((node) => (
              <Card key={node.tenant_id} className="p-6 hover:shadow-lg transition-shadow">
                <div className="flex flex-col sm:flex-row sm:items-center justify-between mb-6 gap-4">
                  <div>
                    <h3 className="text-xl font-bold">{node.name || node.tenant_id}</h3>
                    <p className="text-sm text-foreground/60">
                      Tenant ID: {node.tenant_id} • Port: {node.port}
                    </p>
                  </div>
                  <Badge variant="default" className="text-sm w-fit">
                    <span className="inline-block w-2 h-2 rounded-full bg-green-500 mr-2 animate-pulse"></span>
                    {node.status}
                  </Badge>
                </div>

                <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
                  <div className="flex items-center gap-3 p-4 rounded-lg bg-primary/5 border border-primary/10">
                    <div className="p-3 rounded-lg bg-primary/10">
                      <Database className="w-5 h-5 text-primary" />
                    </div>
                    <div>
                      <p className="text-sm text-foreground/60">Memory Used</p>
                      <p className="text-lg font-semibold">
                        {node.memory_used_mb.toFixed(2)} MB
                      </p>
                      <p className="text-xs text-foreground/50">
                        {node.memory_usage_percent.toFixed(1)}% of {node.memory_limit_mb} MB
                      </p>
                    </div>
                  </div>

                  <div className="flex items-center gap-3 p-4 rounded-lg bg-green-500/5 border border-green-500/10">
                    <div className="p-3 rounded-lg bg-green-500/10">
                      <Users className="w-5 h-5 text-green-500" />
                    </div>
                    <div>
                      <p className="text-sm text-foreground/60">Total Keys</p>
                      <p className="text-lg font-semibold">
                        {node.key_count.toLocaleString()}
                      </p>
                      <p className="text-xs text-foreground/50">Stored items</p>
                    </div>
                  </div>

                  <div className="flex items-center gap-3 p-4 rounded-lg bg-blue-500/5 border border-blue-500/10">
                    <div className="p-3 rounded-lg bg-blue-500/10">
                      <Zap className="w-5 h-5 text-blue-500" />
                    </div>
                    <div>
                      <p className="text-sm text-foreground/60">Connected Clients</p>
                      <p className="text-lg font-semibold">{node.connected_clients}</p>
                      <p className="text-xs text-foreground/50">Active connections</p>
                    </div>
                  </div>

                  <div className="flex items-center gap-3 p-4 rounded-lg bg-purple-500/5 border border-purple-500/10">
                    <div className="p-3 rounded-lg bg-purple-500/10">
                      <TrendingUp className="w-5 h-5 text-purple-500" />
                    </div>
                    <div>
                      <p className="text-sm text-foreground/60">Memory</p>
                      <p className="text-lg font-semibold">
                        {node.memory_used_human}
                      </p>
                      <p className="text-xs text-foreground/50">Human readable</p>
                    </div>
                  </div>
                </div>

                <div className="mt-4 p-3 bg-muted rounded-lg">
                  <p className="text-xs text-foreground/60 mb-1">Connect via CLI:</p>
                  <code className="text-sm font-mono text-primary">
                    {node.redis_cli_command}
                  </code>
                </div>
              </Card>
            ))}
          </div>
        )}
      </div>
    </div>
  );
}
