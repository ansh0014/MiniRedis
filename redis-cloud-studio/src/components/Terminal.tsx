import { useState, useRef, useEffect } from "react";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Play, Terminal as TerminalIcon } from "lucide-react";

interface CommandOutput {
  command: string;
  output: string;
  isError: boolean;
  timestamp: Date;
}

const Terminal = () => {
  const [command, setCommand] = useState("");
  const [history, setHistory] = useState<CommandOutput[]>([
    {
      command: 'SET user:1 "Alice"',
      output: "+OK",
      isError: false,
      timestamp: new Date()
    },
    {
      command: "GET user:1",
      output: '"Alice"',
      isError: false,
      timestamp: new Date()
    }
  ]);
  const [isExecuting, setIsExecuting] = useState(false);
  const outputRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (outputRef.current) {
      outputRef.current.scrollTop = outputRef.current.scrollHeight;
    }
  }, [history]);

  const executeCommand = () => {
    if (!command.trim()) return;

    setIsExecuting(true);

    // Mock command execution
    setTimeout(() => {
      const mockResponses: Record<string, { output: string; isError: boolean }> = {
        ping: { output: "+PONG", isError: false },
        set: { output: "+OK", isError: false },
        get: { output: '"value"', isError: false },
        del: { output: "(integer) 1", isError: false },
        keys: { output: '1) "user:1"\n2) "user:2"\n3) "config"', isError: false },
        flushall: { output: "+OK", isError: false },
        info: { output: "# Server\nredis_version:7.0.0\nuptime_in_seconds:604800", isError: false },
      };

      const cmd = command.toLowerCase().split(" ")[0];
      const response = mockResponses[cmd] || {
        output: cmd.includes("invalid") ? "-ERR unknown command" : "+OK",
        isError: cmd.includes("invalid")
      };

      setHistory(prev => [...prev, {
        command: command,
        output: response.output,
        isError: response.isError,
        timestamp: new Date()
      }]);

      setCommand("");
      setIsExecuting(false);
    }, 300);
  };

  const handleKeyPress = (e: React.KeyboardEvent) => {
    if (e.key === "Enter") {
      executeCommand();
    }
  };

  return (
    <div className="bg-terminal-bg rounded-lg border border-terminal-border overflow-hidden shadow-lg terminal-glow">
      {/* Terminal Header */}
      <div className="flex items-center gap-2 border-b border-terminal-border px-4 py-3 bg-terminal-bg/50">
        <div className="flex gap-1.5">
          <div className="h-3 w-3 rounded-full bg-destructive/80" />
          <div className="h-3 w-3 rounded-full bg-warning/80" />
          <div className="h-3 w-3 rounded-full bg-success/80" />
        </div>
        <div className="flex items-center gap-2 ml-2">
          <TerminalIcon className="h-4 w-4 text-terminal-text/60" />
          <span className="text-sm text-terminal-text/60 font-mono">redis-cli</span>
        </div>
      </div>

      {/* Terminal Output */}
      <div
        ref={outputRef}
        className="h-64 overflow-y-auto p-4 font-mono text-sm space-y-2 scrollbar-thin"
      >
        {history.map((item, index) => (
          <div key={index} className="space-y-1">
            <div className="flex items-start gap-2">
              <span className="text-primary font-bold">&gt;</span>
              <span className="text-muted-foreground">{item.command}</span>
            </div>
            <div className={item.isError ? "text-destructive" : "text-terminal-text"}>
              {item.output.split('\n').map((line, i) => (
                <div key={i} className="ml-4">{line}</div>
              ))}
            </div>
          </div>
        ))}
        {isExecuting && (
          <div className="flex items-center gap-2 ml-4">
            <div className="h-2 w-2 rounded-full bg-terminal-text animate-pulse" />
            <span className="text-terminal-text/60">Executing...</span>
          </div>
        )}
      </div>

      {/* Terminal Input */}
      <div className="flex items-center gap-2 border-t border-terminal-border p-4">
        <span className="text-primary font-mono font-bold">&gt;</span>
        <Input
          value={command}
          onChange={(e) => setCommand(e.target.value)}
          onKeyPress={handleKeyPress}
          placeholder="Enter Redis command..."
          className="flex-1 border-0 bg-transparent font-mono text-terminal-text placeholder:text-terminal-text/40 focus-visible:ring-0 focus-visible:ring-offset-0"
          disabled={isExecuting}
        />
        <Button
          variant="success"
          size="sm"
          onClick={executeCommand}
          loading={isExecuting}
          disabled={!command.trim()}
        >
          <Play className="h-4 w-4" />
          Execute
        </Button>
      </div>
    </div>
  );
};

export default Terminal;
