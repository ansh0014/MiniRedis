#pragma once

#include <string>
#include <map>
#include <mutex>
#include <memory>
#include <chrono>
#include <thread>
#include <vector>

class RedisNode {
public:
    std::string tenantId_;
    int port_;
    int memoryLimitMb_;
    std::string containerId_;
    std::chrono::steady_clock::time_point lastAccessed_;
    bool isRunning_;
    
    RedisNode(const std::string& id, int p, int mem)
        : tenantId_(id), port_(p), memoryLimitMb_(mem), 
          containerId_(""), isRunning_(false),
          lastAccessed_(std::chrono::steady_clock::now()) {}
    
    void touch() {
        lastAccessed_ = std::chrono::steady_clock::now();
    }
    
    int getIdleSeconds() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastAccessed_);
        return static_cast<int>(duration.count());
    }
};

class NodeManager {
public:
    NodeManager();
    ~NodeManager();
    
    bool ensureNodeRunning(const std::string& tenantId, int port, int memoryLimitMb);
    std::string executeCommand(const std::string& tenantId, const std::string& command);
    bool stopNode(const std::string& tenantId);
    bool startNode(const std::string& tenantId, int port, int memoryLimitMb);
    
    std::shared_ptr<RedisNode> getNode(const std::string& tenantId) const;
    
    int getRunningCount() const;
    int getTotalCount() const;
    std::vector<std::string> listNodes() const;
    
private:
    std::map<std::string, std::shared_ptr<RedisNode>> nodes_;
    mutable std::mutex nodesMutex_;
    
    static const int MAX_RUNNING_CONTAINERS = 1000;
    static const int IDLE_TIMEOUT_SECONDS = 300;
    
    std::thread cleanupThread_;
    bool stopCleanup_;
    
    bool createContainer(const std::string& tenantId, int port, int memoryLimitMb);
    bool stopContainer(const std::string& tenantId);
    bool isContainerRunning(const std::string& containerId);
    void cleanupIdleNodes();
    void cleanupLoop();
};