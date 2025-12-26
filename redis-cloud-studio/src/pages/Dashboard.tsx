import { useState, useEffect } from "react";
import { useNavigate } from "react-router-dom";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { Badge } from "@/components/ui/badge";
import MetricCard from "@/components/MetricCard";
const AnyMetricCard = MetricCard as any;
import Terminal from "@/components/Terminal";
import ApiKeyCard from "@/components/ApiKeyCard";

const AnyTerminal = Terminal as any;
import Navbar from "@/components/Navbar";
import {
  Database,
  Activity,
  HardDrive,
  Users,
  Copy,
  Check,
  RefreshCw,
  Power,
  Trash2,
} from "lucide-react";
import { useToast } from "@/hooks/use-toast";

interface Instance {
  id: string;
  host: string;
  port: number;
  password: string;
  apiKey: string;
  status: string;
  memoryUsed: number;
  memoryLimit: number;
  uptime: string;
  commands: number;
}

const Dashboard = () => {
  const navigate = useNavigate();
  const { toast } = useToast();
  const [instance, setInstance] = useState<Instance | null>(null);
  const [loading, setLoading] = useState(true);
  const [copiedField, setCopiedField] = useState<string | null>(null);

  useEffect(() => {
    fetchInstanceData();
  }, []);

  const fetchInstanceData = async () => {
    setLoading(true);
    try {
      const user = JSON.parse(localStorage.getItem("user") || "{}");
      
      if (!user.uid) {
        navigate("/login");
        return;
      }

      const response = await fetch(
        `http://localhost:8080/api/instance/${user.uid}`,
        {
          headers: {
            Authorization: `Bearer ${user.idToken}`,
          },
        }
      );

      if (!response.ok) {
        throw new Error("Failed to fetch instance");
      }

      const data = await response.json();
      setInstance(data);
    } catch (error) {
      console.error("Error fetching instance:", error);
      toast({
        title: "Error",
        description: "Failed to load instance data",
        variant: "destructive",
      });
    } finally {
      setLoading(false);
    }
  };

  const handleCopy = async (text: string, field: string) => {
    await navigator.clipboard.writeText(text);
    setCopiedField(field);
    toast({
      title: "Copied!",
      description: `${field} copied to clipboard`,
    });
    setTimeout(() => setCopiedField(null), 2000);
  };

  const handleRestartInstance = async () => {
    if (!instance) return;

    try {
      const response = await fetch(
        `http://localhost:8080/api/instance/${instance.id}/restart`,
        {
          method: "POST",
          headers: {
            Authorization: `Bearer ${localStorage.getItem("idToken")}`,
          },
        }
      );

      if (!response.ok) throw new Error("Failed to restart instance");

      toast({
        title: "Instance Restarted",
        description: "Your Redis instance has been restarted successfully",
      });

      fetchInstanceData();
    } catch (error) {
      toast({
        title: "Error",
        description: "Failed to restart instance",
        variant: "destructive",
      });
    }
  };

  const handleDeleteInstance = async () => {
    if (!instance) return;

    if (!confirm("Are you sure you want to delete this instance?")) return;

    try {
      const response = await fetch(
        `http://localhost:8080/api/instance/${instance.id}`,
        {
          method: "DELETE",
          headers: {
            Authorization: `Bearer ${localStorage.getItem("idToken")}`,
          },
        }
      );

      if (!response.ok) throw new Error("Failed to delete instance");

      toast({
        title: "Instance Deleted",
        description: "Your Redis instance has been deleted",
      });

      navigate("/");
    } catch (error) {
      toast({
        title: "Error",
        description: "Failed to delete instance",
        variant: "destructive",
      });
    }
  };

  if (loading) {
    return (
      <div className="min-h-screen bg-gradient-to-br from-slate-900 via-slate-800 to-slate-900">
        <Navbar />
        <div className="flex items-center justify-center h-screen">
          <div className="flex items-center gap-3 text-cyan-400">
            <RefreshCw className="h-8 w-8 animate-spin" />
            <span className="text-xl font-[Orbitron]">Loading your Redis instance...</span>
          </div>
        </div>
      </div>
    );
  }

  if (!instance) {
    return (
      <div className="min-h-screen bg-gradient-to-br from-slate-900 via-slate-800 to-slate-900">
        <Navbar />
        <div className="flex items-center justify-center h-screen">
          <Card className="bg-slate-900/50 border-cyan-400/30 max-w-md">
            <CardHeader>
              <CardTitle className="text-cyan-400 font-[Orbitron]">No Instance Found</CardTitle>
            </CardHeader>
            <CardContent>
              <p className="text-cyan-100/70 mb-4">
                You don't have a Redis instance yet. Create one to get started.
              </p>
              <Button
                onClick={() => navigate("/register")}
                className="w-full bg-gradient-to-r from-cyan-500 to-blue-500"
              >
                Create Instance
              </Button>
            </CardContent>
          </Card>
        </div>
      </div>
    );
  }

  const connectionString = `redis://:${instance.password}@${instance.host}:${instance.port}`;
  const memoryPercent = (instance.memoryUsed / instance.memoryLimit) * 100;

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-900 via-slate-800 to-slate-900">
      <Navbar />

      <div className="container mx-auto p-6 pt-24">
        {/* Header */}
        <div className="flex justify-between items-center mb-8">
          <div>
            <h1 className="text-4xl font-[Orbitron] font-bold text-white mb-2 flex items-center gap-3">
              <Database className="h-10 w-10 text-cyan-400" />
              Redis Instance
            </h1>
            <div className="flex items-center gap-2">
              <Badge
                variant={instance.status === "active" ? "default" : "destructive"}
                className="bg-green-500/20 text-green-400 border-green-400/50"
              >
                <Power className="h-3 w-3 mr-1" />
                {instance.status.toUpperCase()}
              </Badge>
              <span className="text-cyan-100/60">Uptime: {instance.uptime}</span>
            </div>
          </div>

          <div className="flex gap-3">
            <Button
              variant="outline"
              onClick={handleRestartInstance}
              className="border-cyan-400/50 text-cyan-400 hover:bg-cyan-400/10"
            >
              <RefreshCw className="h-4 w-4 mr-2" />
              Restart
            </Button>
            <Button
              variant="outline"
              onClick={handleDeleteInstance}
              className="border-red-400/50 text-red-400 hover:bg-red-400/10"
            >
              <Trash2 className="h-4 w-4 mr-2" />
              Delete
            </Button>
          </div>
        </div>

        {/* Metrics */}
        <div className="grid grid-cols-1 md:grid-cols-4 gap-6 mb-8">
          <AnyMetricCard
            title="Memory Usage"
            value={`${instance.memoryUsed}MB / ${instance.memoryLimit}MB`}
            icon={HardDrive}
            trend={memoryPercent}
          />
          <AnyMetricCard
            title="Commands/sec"
            value={instance.commands.toLocaleString()}
            icon={Activity}
            trend={12}
          />
          <AnyMetricCard
            title="Connected Clients"
            value="1"
            icon={Users}
            trend={0}
          />
          <AnyMetricCard
            title="Keys"
            value="0"
            icon={Database}
            trend={0}
          />
        </div>

        {/* Main Content */}
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
          {/* Left Column - Connection Details */}
          <div className="lg:col-span-1 space-y-6">
            <Card className="bg-slate-900/50 border-cyan-400/30 backdrop-blur-xl">
              <CardHeader>
                <CardTitle className="text-cyan-400 font-[Orbitron] flex items-center gap-2">
                  <Database className="h-5 w-5" />
                  Connection Details
                </CardTitle>
              </CardHeader>
              <CardContent className="space-y-4">
                <div>
                  <label className="text-cyan-100/60 text-sm">Host</label>
                  <div className="flex items-center gap-2 mt-1">
                    <code className="flex-1 bg-slate-800/50 px-3 py-2 rounded text-cyan-100 font-mono text-sm">
                      {instance.host}
                    </code>
                    <Button
                      size="sm"
                      variant="ghost"
                      onClick={() => handleCopy(instance.host, "Host")}
                      className="text-cyan-400 hover:bg-cyan-400/10"
                    >
                      {copiedField === "Host" ? (
                        <Check className="h-4 w-4" />
                      ) : (
                        <Copy className="h-4 w-4" />
                      )}
                    </Button>
                  </div>
                </div>

                <div>
                  <label className="text-cyan-100/60 text-sm">Port</label>
                  <div className="flex items-center gap-2 mt-1">
                    <code className="flex-1 bg-slate-800/50 px-3 py-2 rounded text-cyan-100 font-mono text-sm">
                      {instance.port}
                    </code>
                    <Button
                      size="sm"
                      variant="ghost"
                      onClick={() => handleCopy(instance.port.toString(), "Port")}
                      className="text-cyan-400 hover:bg-cyan-400/10"
                    >
                      {copiedField === "Port" ? (
                        <Check className="h-4 w-4" />
                      ) : (
                        <Copy className="h-4 w-4" />
                      )}
                    </Button>
                  </div>
                </div>

                <div>
                  <label className="text-cyan-100/60 text-sm">Password</label>
                  <div className="flex items-center gap-2 mt-1">
                    <code className="flex-1 bg-slate-800/50 px-3 py-2 rounded text-cyan-100 font-mono text-sm">
                      {"•".repeat(20)}
                    </code>
                    <Button
                      size="sm"
                      variant="ghost"
                      onClick={() => handleCopy(instance.password, "Password")}
                      className="text-cyan-400 hover:bg-cyan-400/10"
                    >
                      {copiedField === "Password" ? (
                        <Check className="h-4 w-4" />
                      ) : (
                        <Copy className="h-4 w-4" />
                      )}
                    </Button>
                  </div>
                </div>

                <div>
                  <label className="text-cyan-100/60 text-sm">Connection String</label>
                  <div className="flex items-center gap-2 mt-1">
                    <code className="flex-1 bg-slate-800/50 px-3 py-2 rounded text-cyan-100 font-mono text-sm break-all">
                      {connectionString}
                    </code>
                    <Button
                      size="sm"
                      variant="ghost"
                      onClick={() => handleCopy(connectionString, "Connection String")}
                      className="text-cyan-400 hover:bg-cyan-400/10"
                    >
                      {copiedField === "Connection String" ? (
                        <Check className="h-4 w-4" />
                      ) : (
                        <Copy className="h-4 w-4" />
                      )}
                    </Button>
                  </div>
                </div>
              </CardContent>
            </Card>

            <ApiKeyCard apiKey={instance.apiKey} />
          </div>

          {/* Right Column - Terminal */}
          <div className="lg:col-span-2">
            <Card className="bg-slate-900/50 border-cyan-400/30 backdrop-blur-xl">
              <CardHeader>
                <CardTitle className="text-cyan-400 font-[Orbitron]">
                  Redis Terminal
                </CardTitle>
              </CardHeader>
              <CardContent>
                <AnyTerminal
                  host={instance.host}
                  port={instance.port}
                  password={instance.password}
                />
              </CardContent>
            </Card>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Dashboard;
