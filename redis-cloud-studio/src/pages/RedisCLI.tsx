import React, { useState, useEffect } from 'react';
import { Send, Terminal, ChevronDown } from 'lucide-react';
import { toast } from 'react-toastify';
import { api } from '../lib/api';

interface CommandHistory {
  command: string;
  output: string;
  timestamp: Date;
}

interface RedisNode {
  tenant_id: string;
  port: number;
  status: string;
  created_at: string;
}

export default function RedisCLI() {
  const [command, setCommand] = useState('');
  const [history, setHistory] = useState<CommandHistory[]>([]);
  const [nodes, setNodes] = useState<RedisNode[]>([]);
  const [selectedNode, setSelectedNode] = useState<RedisNode | null>(null);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    loadNodes();
  }, []);

  const loadNodes = async () => {
    try {
      const nodeList = await api.listNodes();
      console.log('📋 Loaded nodes:', nodeList);
      setNodes(nodeList);
      
      if (nodeList.length > 0 && !selectedNode) {
        setSelectedNode(nodeList[0]);
      }
    } catch (error: any) {
      console.error('❌ Failed to load nodes:', error);
      toast.error('Failed to load Redis instances');
    }
  };

  const handleExecute = async () => {
    if (!command.trim()) {
      toast.error('Please enter a command');
      return;
    }

    if (!selectedNode) {
      toast.error('Please select a Redis instance');
      return;
    }

    const commandEntry: CommandHistory = {
      command: command.trim(),
      output: 'Executing...',
      timestamp: new Date()
    };
    
    setHistory(prev => [...prev, commandEntry]);
    setCommand('');
    setLoading(true);

    try {
      console.log('🚀 Executing command:', {
        tenantId: selectedNode.tenant_id,
        port: selectedNode.port,
        command: command.trim()
      });

      const result = await api.executeCommand(
        selectedNode.tenant_id,
        command.trim()
      );

      console.log('✅ Command result:', result);

      setHistory(prev => {
        const newHistory = [...prev];
        newHistory[newHistory.length - 1] = {
          ...commandEntry,
          output: result.result || '(nil)'
        };
        return newHistory;
      });

      toast.success('Command executed');
    } catch (error: any) {
      console.error('❌ Command failed:', error);
      
      setHistory(prev => {
        const newHistory = [...prev];
        newHistory[newHistory.length - 1] = {
          ...commandEntry,
          output: `Error: ${error.message || 'Command failed'}`
        };
        return newHistory;
      });

      toast.error('Failed to execute command');
    } finally {
      setLoading(false);
    }
  };

  const handleKeyPress = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleExecute();
    }
  };

  return (
    <div className="max-w-6xl mx-auto p-6 space-y-6">
      {/* Header */}
      <div className="bg-gradient-to-r from-gray-900 to-gray-800 rounded-xl p-6 border border-cyan-500/20">
        <div className="flex items-center gap-3 mb-2">
          <Terminal className="w-6 h-6 text-cyan-400" />
          <h1 className="text-2xl font-bold text-white">Redis CLI</h1>
        </div>
        <p className="text-gray-400">Execute commands on your instances</p>
      </div>

      {/* Node Selector */}
      <div className="bg-gray-900 rounded-xl p-4 border border-cyan-500/20">
        <label className="block text-sm font-medium text-gray-300 mb-2">
          Select Redis Instance
        </label>
        <div className="relative">
          <select
            value={selectedNode?.tenant_id || ''}
            onChange={(e) => {
              const node = nodes.find(n => n.tenant_id === e.target.value);
              setSelectedNode(node || null);
            }}
            className="w-full bg-gray-800 border border-gray-700 rounded-lg px-4 py-3 text-white appearance-none cursor-pointer hover:border-cyan-500/50 transition-colors"
          >
            <option value="">Select an instance...</option>
            {nodes.map((node) => (
              <option key={node.tenant_id} value={node.tenant_id}>
                (Port {node.port}) - {node.status}
              </option>
            ))}
          </select>
          <ChevronDown className="absolute right-3 top-1/2 -translate-y-1/2 w-5 h-5 text-gray-400 pointer-events-none" />
        </div>
      </div>

      {/* Command History */}
      <div className="bg-gray-900 rounded-xl border border-cyan-500/20 overflow-hidden">
        <div className="bg-gray-800 px-4 py-2 border-b border-gray-700">
          <p className="text-sm text-gray-400">Command History</p>
        </div>
        <div className="p-4 h-96 overflow-y-auto font-mono text-sm bg-black/50">
          {history.length === 0 ? (
            <p className="text-gray-500 text-center py-8">
              No commands executed yet. Try: SET mykey "Hello World"
            </p>
          ) : (
            <div className="space-y-4">
              {history.map((entry, idx) => (
                <div key={idx} className="space-y-1">
                  <div className="flex items-center gap-2 text-cyan-400">
                    <span className="text-gray-500">
                      {entry.timestamp.toLocaleTimeString()}
                    </span>
                    <span>&gt;</span>
                    <span className="font-semibold">{entry.command}</span>
                  </div>
                  <div className="pl-6 text-gray-300 whitespace-pre-wrap">
                    {entry.output}
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>
      </div>

      {/* Command Input */}
      <div className="bg-gray-900 rounded-xl border border-cyan-500/20 p-4">
        <label className="block text-sm font-medium text-gray-300 mb-2">
          Enter Redis Command (e.g., SET key value)
        </label>
        <div className="flex gap-3">
          <input
            type="text"
            value={command}
            onChange={(e) => setCommand(e.target.value)}
            onKeyPress={handleKeyPress}
            placeholder="SET mykey 'Hello World'"
            disabled={!selectedNode || loading}
            className="flex-1 bg-gray-800 border border-gray-700 rounded-lg px-4 py-3 text-white placeholder-gray-500 focus:outline-none focus:border-cyan-500 disabled:opacity-50 disabled:cursor-not-allowed"
          />
          <button
            onClick={handleExecute}
            disabled={!selectedNode || !command.trim() || loading}
            className="bg-cyan-500 hover:bg-cyan-600 disabled:bg-gray-700 disabled:cursor-not-allowed text-white px-6 py-3 rounded-lg font-medium transition-colors flex items-center gap-2"
          >
            <Send className="w-5 h-5" />
            {loading ? 'Executing...' : 'Execute'}
          </button>
        </div>
      </div>
    </div>
  );
}