import { useState } from "react";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { ScrollArea } from "@/components/ui/scroll-area";
import { Send, Terminal as TerminalIcon } from "lucide-react";
import { api, type RedisNode } from "@/lib/api";
import { toast } from "sonner";

interface TerminalProps {
  nodes: RedisNode[];
}

interface CommandOutput {
  command: string;
  result: string;
  timestamp: Date;
  tenantId: string;
}

export default function Terminal({ nodes }: TerminalProps) {
  const [command, setCommand] = useState("");
  const [output, setOutput] = useState<CommandOutput[]>([]);
  const [selectedNode, setSelectedNode] = useState<string>(nodes[0]?.tenant_id || "");
  const [executing, setExecuting] = useState(false);

  const handleExecute = async () => {
    if (!command.trim() || !selectedNode) return;

    setExecuting(true);
    try {
      const result = await api.executeCommand(selectedNode, command);
      
      setOutput((prev) => [
        ...prev,
        {
          command,
          result: result.result,
          timestamp: new Date(),
          tenantId: selectedNode,
        },
      ]);
      
      setCommand("");
    } catch (error: any) {
      toast.error(error.message || "Failed to execute command");
      setOutput((prev) => [
        ...prev,
        {
          command,
          result: `Error: ${error.message}`,
          timestamp: new Date(),
          tenantId: selectedNode,
        },
      ]);
    } finally {
      setExecuting(false);
    }
  };

  return (
    <div className="space-y-4">
      {/* Node Selector */}
      <div className="flex items-center gap-2">
        <TerminalIcon className="h-4 w-4 text-muted-foreground" />
        <select
          value={selectedNode}
          onChange={(e) => setSelectedNode(e.target.value)}
          className="flex-1 h-10 rounded-md border border-input bg-background px-3 py-2 text-sm ring-offset-background focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-offset-2"
        >
          {nodes.map((node) => (
            <option key={node.tenant_id} value={node.tenant_id}>
              {node.tenant_id} (Port {node.port}) - {node.status}
            </option>
          ))}
        </select>
      </div>

      {/* Output */}
      <div className="rounded-lg border bg-slate-950">
        <ScrollArea className="h-[400px] p-4">
          <div className="font-mono text-sm space-y-3">
            {output.length === 0 ? (
              <p className="text-slate-500">
                No commands executed yet. Try: SET mykey "Hello World"
              </p>
            ) : (
              output.map((item, i) => (
                <div key={i} className="space-y-1">
                  <div className="text-blue-400">
                    <span className="text-slate-500">
                      [{item.timestamp.toLocaleTimeString()}]
                    </span>{" "}
                    {item.tenantId} &gt; <span className="text-yellow-400">{item.command}</span>
                  </div>
                  <div className="text-green-400 pl-4 whitespace-pre-wrap">
                    {item.result}
                  </div>
                </div>
              ))
            )}
          </div>
        </ScrollArea>
      </div>

      {/* Input */}
      <div className="flex gap-2">
        <Input
          placeholder="Enter Redis command (e.g., SET key value)"
          value={command}
          onChange={(e) => setCommand(e.target.value)}
          onKeyDown={(e) => {
            if (e.key === "Enter" && !e.shiftKey) {
              e.preventDefault();
              handleExecute();
            }
          }}
          disabled={executing || !selectedNode}
          className="font-mono flex-1"
        />
        <Button 
          onClick={handleExecute} 
          disabled={executing || !command.trim() || !selectedNode}
          size="icon"
        >
          <Send className="h-4 w-4" />
        </Button>
      </div>
    </div>
  );
}
