import { useState, useEffect } from "react";
import { useNavigate } from "react-router-dom";
import Navbar from "@/components/Navbar";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Badge } from "@/components/ui/badge";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs";
import Terminal from "@/components/Terminal";
import { Play, Square, Trash2, Plus, Database, Activity, Clock, Server } from "lucide-react";
import { api, type RedisNode, type Tenant, type User } from "@/lib/api";
import { toast } from "sonner";

export default function Dashboard() {
  const navigate = useNavigate();
  const [user, setUser] = useState<User | null>(null);
  const [nodes, setNodes] = useState<RedisNode[]>([]);
  const [tenants, setTenants] = useState<Tenant[]>([]);
  const [loading, setLoading] = useState(true);
  const [creatingInstance, setCreatingInstance] = useState(false);
  const [newInstanceName, setNewInstanceName] = useState("");
  const [newInstancePort, setNewInstancePort] = useState(6380);

  useEffect(() => {
    loadUserAndData();
  }, []);

  const loadUserAndData = async () => {
    try {
      const userData = await api.me();
      setUser(userData);
      await loadNodes();
    } catch (error) {
      navigate("/login");
    } finally {
      setLoading(false);
    }
  };

  const loadNodes = async () => {
    try {
      const nodeList = await api.listNodes();
      setNodes(nodeList);
    } catch (error) {
      toast.error("Failed to load instances");
    }
  };

  const handleCreateInstance = async () => {
    if (!newInstanceName.trim()) {
      toast.error("Instance name is required");
      return;
    }

    setCreatingInstance(true);
    try {
      const tenant = await api.createTenant(newInstanceName, 40);

      toast.success(`Instance "${newInstanceName}" created on port ${tenant.port}!`);

      setNewInstanceName("");
      setNewInstancePort(tenant.port + 1);

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
      toast.success("Node started successfully");
      await loadNodes();
    } catch (error: any) {
      toast.error(error.message || "Failed to start node");
    }
  };

  const handleStopNode = async (tenantId: string) => {
    try {
      await api.stopNode(tenantId);
      toast.success("Node stopped successfully");
      await loadNodes();
    } catch (error: any) {
      toast.error(error.message || "Failed to stop node");
    }
  };

  if (loading) {
    return (
      <div className="flex items-center justify-center h-screen">
        <div className="animate-spin rounded-full h-32 w-32 border-b-2 border-primary"></div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-background">
      <Navbar user={user} />
      
      <main className="container mx-auto p-6 space-y-6">
        <div className="flex justify-between items-center">
          <div>
            <h1 className="text-3xl font-bold">Dashboard</h1>
            <p className="text-muted-foreground">Manage your Redis instances</p>
          </div>
        </div>

        <div className="grid gap-4 md:grid-cols-3">
          <Card>
            <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
              <CardTitle className="text-sm font-medium">Total Instances</CardTitle>
              <Database className="h-4 w-4 text-muted-foreground" />
            </CardHeader>
            <CardContent>
              <div className="text-2xl font-bold">{nodes.length}</div>
            </CardContent>
          </Card>
          
          <Card>
            <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
              <CardTitle className="text-sm font-medium">Active Nodes</CardTitle>
              <Activity className="h-4 w-4 text-muted-foreground" />
            </CardHeader>
            <CardContent>
              <div className="text-2xl font-bold">
                {nodes.filter(n => n.status === "running").length}
              </div>
            </CardContent>
          </Card>
          
          <Card>
            <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
              <CardTitle className="text-sm font-medium">Stopped Nodes</CardTitle>
              <Server className="h-4 w-4 text-muted-foreground" />
            </CardHeader>
            <CardContent>
              <div className="text-2xl font-bold">
                {nodes.filter(n => n.status === "stopped").length}
              </div>
            </CardContent>
          </Card>
        </div>

        <Card>
          <CardHeader>
            <CardTitle>Create New Instance</CardTitle>
            <CardDescription>Deploy a new Redis instance</CardDescription>
          </CardHeader>
          <CardContent>
            <div className="flex gap-4">
              <Input
                placeholder="Instance name (e.g., my-app-cache)"
                value={newInstanceName}
                onChange={(e) => setNewInstanceName(e.target.value)}
                disabled={creatingInstance}
              />
              <Input
                type="number"
                placeholder="Port"
                value={newInstancePort}
                onChange={(e) => setNewInstancePort(Number(e.target.value))}
                disabled={creatingInstance}
                className="w-32"
              />
              <Button 
                onClick={handleCreateInstance}
                disabled={creatingInstance || !newInstanceName.trim()}
              >
                <Plus className="mr-2 h-4 w-4" />
                {creatingInstance ? "Creating..." : "Create"}
              </Button>
            </div>
          </CardContent>
        </Card>

        <Card>
          <CardHeader>
            <CardTitle>Your Instances</CardTitle>
            <CardDescription>Manage your Redis instances</CardDescription>
          </CardHeader>
          <CardContent>
            {nodes.length === 0 ? (
              <div className="text-center py-8 text-muted-foreground">
                No instances yet. Create your first instance above!
              </div>
            ) : (
              <div className="space-y-4">
                {nodes.map((node) => (
                  <div
                    key={node.tenant_id}
                    className="flex items-center justify-between p-4 border rounded-lg"
                  >
                    <div className="flex items-center gap-4">
                      <Database className="h-8 w-8 text-primary" />
                      <div>
                        <h3 className="font-semibold">{node.tenant_id}</h3>
                        <p className="text-sm text-muted-foreground">
                          Port: {node.port} • Created: {new Date(node.created_at).toLocaleDateString()}
                        </p>
                      </div>
                    </div>
                    
                    <div className="flex items-center gap-3">
                      <Badge variant={node.status === "running" ? "default" : "secondary"}>
                        {node.status}
                      </Badge>
                      
                      {node.status === "running" ? (
                        <Button
                          size="sm"
                          variant="outline"
                          onClick={() => handleStopNode(node.tenant_id)}
                        >
                          <Square className="h-4 w-4" />
                        </Button>
                      ) : (
                        <Button
                          size="sm"
                          onClick={() => handleStartNode(node.tenant_id)}
                        >
                          <Play className="h-4 w-4" />
                        </Button>
                      )}
                    </div>
                  </div>
                ))}
              </div>
            )}
          </CardContent>
        </Card>

        {nodes.length > 0 && (
          <Card>
            <CardHeader>
              <CardTitle>Redis CLI</CardTitle>
              <CardDescription>Execute commands on your instances</CardDescription>
            </CardHeader>
            <CardContent>
              <Terminal nodes={nodes} />
            </CardContent>
          </Card>
        )}
      </main>
    </div>
  );
}
