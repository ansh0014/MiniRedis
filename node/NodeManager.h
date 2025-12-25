#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>

// Key-Value entry with TTL support
struct KVEntry {
    std::string value;
    std::chrono::steady_clock::time_point expiry;
    bool hasExpiry;
    
    // ADD DEFAULT CONSTRUCTOR
    KVEntry() : value(""), hasExpiry(false) {}
    
    KVEntry(const std::string& val) 
        : value(val), hasExpiry(false) {}
    
    KVEntry(const std::string& val, int ttlSeconds)
        : value(val), 
          expiry(std::chrono::steady_clock::now() + std::chrono::seconds(ttlSeconds)),
          hasExpiry(true) {}
    
    bool isExpired() const {
        return hasExpiry && std::chrono::steady_clock::now() > expiry;
    }
};

// In-Memory Redis Node (40MB limit per tenant)
class RedisNode {
public:
    RedisNode(const std::string& tenantId, int port, int memoryLimitMb);
    ~RedisNode();

    // Redis commands
    std::string set(const std::string& key, const std::string& value, int ttl = 0);
    std::string get(const std::string& key);
    std::string del(const std::string& key);
    std::string exists(const std::string& key);
    std::string keys(const std::string& pattern);
    std::string flushall();
    std::string ping();
    
    // Node info
    std::string getTenantId() const { return tenantId_; }
    int getPort() const { return port_; }
    size_t getMemoryUsage() const;
    size_t getKeyCount() const;
    bool isRunning() const { return running_; }
    
    void start();
    void stop();

private:
    std::string tenantId_;
    int port_;
    size_t memoryLimitBytes_;
    std::atomic<bool> running_;
    
    // In-memory storage (thread-safe)
    std::unordered_map<std::string, KVEntry> storage_;
    mutable std::mutex storageMutex_;
    
    // TTL sweeper thread (removes expired keys)
    std::thread sweeperThread_;
    void ttlSweeperLoop();
    
    // Memory management (LRU eviction)
    bool checkMemoryLimit(size_t additionalBytes);
    void evictLRU();
};

// Node Manager - manages multiple tenant nodes
class NodeManager {
public:
    NodeManager();
    ~NodeManager();

    // Start a node for tenant
    bool startNode(const std::string& tenantId, int port, int memoryLimitMb = 40);
    
    // Stop a node
    bool stopNode(const std::string& tenantId);
    
    // Get node by tenant ID
    std::shared_ptr<RedisNode> getNode(const std::string& tenantId);
    
    // Execute Redis command on tenant's node
    std::string executeCommand(const std::string& tenantId, const std::string& command);
    
    // List all nodes
    std::vector<std::string> listNodes();
    
    void stopAllNodes();

private:
    std::unordered_map<std::string, std::shared_ptr<RedisNode>> nodes_;
    mutable std::mutex nodesMutex_;
};