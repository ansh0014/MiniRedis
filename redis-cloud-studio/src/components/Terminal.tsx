import { useState, useRef, useEffect } from "react";
import { ScrollArea } from "@/components/ui/scroll-area";
import { Send } from "lucide-react";
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
  isError?: boolean;
}

export default function Terminal({ nodes }: TerminalProps) {
  const [command, setCommand] = useState("");
  const [output, setOutput] = useState<CommandOutput[]>([]);
  const [selectedNode, setSelectedNode] = useState<string>(nodes[0]?.tenant_id || "");
  const [executing, setExecuting] = useState(false);
  const [history, setHistory] = useState<string[]>([]);
  const [historyIdx, setHistoryIdx] = useState(-1);
  const bottomRef = useRef<HTMLDivElement>(null);
  const inputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [output]);

  const handleExecute = async () => {
    if (!command.trim() || !selectedNode) return;
    setExecuting(true);
    const cmd = command.trim();
    setHistory(prev => [cmd, ...prev.slice(0, 49)]);
    setHistoryIdx(-1);

    try {
      const result = await api.executeCommand(selectedNode, cmd);
      setOutput(prev => [...prev, { command: cmd, result: result.result, timestamp: new Date(), tenantId: selectedNode }]);
      setCommand("");
    } catch (error: any) {
      toast.error(error.message || "Failed to execute command");
      setOutput(prev => [...prev, {
        command: cmd,
        result: `ERR ${error.message || "command failed"}`,
        timestamp: new Date(),
        tenantId: selectedNode,
        isError: true,
      }]);
      setCommand("");
    } finally {
      setExecuting(false);
    }
  };

  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === "Enter" && !e.shiftKey) {
      e.preventDefault();
      handleExecute();
    } else if (e.key === "ArrowUp") {
      e.preventDefault();
      const next = Math.min(historyIdx + 1, history.length - 1);
      setHistoryIdx(next);
      setCommand(history[next] || "");
    } else if (e.key === "ArrowDown") {
      e.preventDefault();
      const next = Math.max(historyIdx - 1, -1);
      setHistoryIdx(next);
      setCommand(next === -1 ? "" : history[next]);
    }
  };

  return (
    <div className="space-y-3">
      {/* Node selector */}
      <div className="flex items-center gap-3">
        <span className="label-tag shrink-0">Target:</span>
        <select
          value={selectedNode}
          onChange={(e) => setSelectedNode(e.target.value)}
          className="flex-1 h-8 rounded border border-white/8 bg-black/50 px-3 text-xs mono text-emerald-400/80 focus:outline-none focus:border-emerald-500/40"
        >
          {nodes.map((node) => (
            <option key={node.tenant_id} value={node.tenant_id} className="bg-gray-900 text-white">
              {node.tenant_id} :{node.port} [{node.status}]
            </option>
          ))}
        </select>
        <button
          onClick={() => setOutput([])}
          className="px-3 h-8 rounded border border-white/8 bg-white/3 hover:bg-white/8 text-white/30 hover:text-white/60 text-xs mono transition-all"
        >
          CLR
        </button>
      </div>

      {/* Output window */}
      <div className="rounded-lg border border-emerald-500/10 bg-black/60">
        <ScrollArea className="h-72 p-4" onClick={() => inputRef.current?.focus()}>
          <div className="font-mono text-xs space-y-2 min-h-full">
            {output.length === 0 ? (
              <div className="text-white/20 pt-2">
                <span className="text-emerald-400/40">$ </span>
                No commands yet — try: <span className="text-emerald-400/60">PING</span>, <span className="text-emerald-400/60">SET key val</span>, <span className="text-emerald-400/60">KEYS *</span>
              </div>
            ) : (
              output.map((item, i) => (
                <div key={i} className="space-y-0.5">
                  <div className="flex items-start gap-2">
                    <span className="text-white/20 shrink-0">{item.timestamp.toLocaleTimeString('en', { hour12: false })}</span>
                    <span className="text-emerald-400/50 shrink-0">{item.tenantId}</span>
                    <span className="text-emerald-400/40 shrink-0">&gt;</span>
                    <span className="text-yellow-300/80">{item.command}</span>
                  </div>
                  <div className={`pl-[calc(4rem+8px)] ${item.isError ? 'text-red-400/80' : 'text-emerald-400/90'} whitespace-pre-wrap leading-relaxed`}>
                    {item.result}
                  </div>
                </div>
              ))
            )}
            <div ref={bottomRef} />
          </div>
        </ScrollArea>
      </div>

      {/* Input row */}
      <div className="flex items-center gap-2 bg-black/60 border border-emerald-500/15 rounded-lg px-4 py-2.5">
        <span className="text-emerald-400/40 mono text-xs shrink-0">$</span>
        <input
          ref={inputRef}
          id="redis-cli-input"
          placeholder={selectedNode ? `Command for ${selectedNode}...` : "Select a node first"}
          value={command}
          onChange={(e) => setCommand(e.target.value)}
          onKeyDown={handleKeyDown}
          disabled={executing || !selectedNode}
          className="flex-1 bg-transparent text-xs mono text-emerald-300/80 placeholder:text-white/15 focus:outline-none disabled:opacity-40"
        />
        <button
          onClick={handleExecute}
          disabled={executing || !command.trim() || !selectedNode}
          className="p-1.5 rounded bg-emerald-500/15 border border-emerald-500/20 text-emerald-400 hover:bg-emerald-500/25 hover:border-emerald-500/40 disabled:opacity-30 disabled:cursor-not-allowed transition-all"
        >
          <Send className="h-3.5 w-3.5" />
        </button>
      </div>
    </div>
  );
}
