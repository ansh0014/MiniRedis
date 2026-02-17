#include "NodeManager.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
    #include <windows.h>
    #define popen _popen
    #define pclose _pclose
    #define NULL_REDIRECT " >nul 2>&1"
    #define SHELL_AND " & "
#else
    #define NULL_REDIRECT " >/dev/null 2>&1"
    #define SHELL_AND " && "
#endif

NodeManager::NodeManager() : stopCleanup_(false) {
    std::cout << "[NodeManager] Initializing with smart pooling" << std::endl;
    std::cout << "[NodeManager] Max running containers: " << MAX_RUNNING_CONTAINERS << std::endl;
    std::cout << "[NodeManager] Idle timeout: " << IDLE_TIMEOUT_SECONDS << " seconds" << std::endl;
    
    cleanupThread_ = std::thread(&NodeManager::cleanupLoop, this);
}

NodeManager::~NodeManager() {
    stopCleanup_ = true;
    if (cleanupThread_.joinable()) {
        cleanupThread_.join();
    }
}

bool NodeManager::ensureNodeRunning(const std::string& tenantId, int port, int memoryLimitMb) {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    
    auto it = nodes_.find(tenantId);
    if (it != nodes_.end()) {
        auto& node = it->second;
        node->touch();
        
        if (node->isRunning_ && isContainerRunning(node->containerId_)) {
            return true;
        }
        
        if (!node->containerId_.empty()) {
            std::string cmd = "docker start " + node->containerId_ + " 2>&1";
            FILE* pipe = popen(cmd.c_str(), "r");
            if (pipe) {
                int result = pclose(pipe);
                if (result == 0) {
                    node->isRunning_ = true;
                    std::cout << "[NodeManager] Restarted container: " << tenantId << std::endl;
                    return true;
                }
            }
        }
    }
    
    int runningCount = getRunningCount();
    if (runningCount >= MAX_RUNNING_CONTAINERS) {
        std::cout << "[NodeManager] At capacity (" << runningCount << "/" 
                  << MAX_RUNNING_CONTAINERS << "), cleaning up idle nodes" << std::endl;
        cleanupIdleNodes();
    }
    
    return createContainer(tenantId, port, memoryLimitMb);
}

std::string NodeManager::executeCommand(const std::string& tenantId, const std::string& command) {
    auto it = nodes_.find(tenantId);
    if (it == nodes_.end()) {
        return "ERROR: Tenant not found";
    }
    
    auto& node = it->second;
    
    if (!ensureNodeRunning(tenantId, node->port_, node->memoryLimitMb_)) {
        return "ERROR: Failed to start container";
    }
    
    std::ostringstream cmd;
    cmd << "docker exec redis-tenant-" << tenantId 
        << " redis-cli " << command << " 2>&1";
    
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        return "ERROR: Failed to execute command";
    }
    
    char buffer[4096];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    
    node->touch();
    
    return result;
}

bool NodeManager::createContainer(const std::string& tenantId, int port, int memoryLimitMb) {
    std::cout << "[NodeManager] Creating container for tenant: " << tenantId << std::endl;
    
    std::ostringstream cmd;
    std::string containerName = "redis-tenant-" + tenantId;
    std::string volumeName = containerName + "-data";
    
    cmd << "docker volume create " << volumeName << NULL_REDIRECT << SHELL_AND;
    cmd << "docker run -d "
        << "--name " << containerName << " "
        << "--network miniredis_miniredis-network "
        << "-p " << port << ":6379 "
        << "-v " << volumeName << ":/data "
        << "redis:7-alpine "
        << "redis-server "
        << "--maxmemory " << memoryLimitMb << "mb "
        << "--maxmemory-policy allkeys-lru "
        << "--appendonly yes "
        << "--protected-mode no";
    
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        std::cerr << "[NodeManager] Failed to execute docker command" << std::endl;
        return false;
    }
    
    char buffer[256];
    std::string containerId;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        containerId = buffer;
        containerId.erase(std::remove(containerId.begin(), containerId.end(), '\n'), 
                         containerId.end());
        containerId.erase(std::remove(containerId.begin(), containerId.end(), '\r'), 
                         containerId.end());
    }
    
    int returnCode = pclose(pipe);
    
    if (returnCode == 0 && !containerId.empty()) {
        auto node = std::make_shared<RedisNode>(tenantId, port, memoryLimitMb);
        node->containerId_ = containerId;
        node->isRunning_ = true;
        nodes_[tenantId] = node;
        
        std::cout << "[NodeManager] Container created: " << containerId.substr(0, 12) 
                  << " (Total running: " << getRunningCount() << ")" << std::endl;
        return true;
    }
    
    std::cerr << "[NodeManager] Failed to create container (exit code: " << returnCode << ")" << std::endl;
    return false;
}

bool NodeManager::stopContainer(const std::string& tenantId) {
    auto it = nodes_.find(tenantId);
    if (it == nodes_.end()) {
        return false;
    }
    
    std::string cmd = "docker stop redis-tenant-" + tenantId + " 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return false;
    }
    
    int result = pclose(pipe);
    if (result == 0) {
        it->second->isRunning_ = false;
        std::cout << "[NodeManager] Container stopped: " << tenantId << std::endl;
        return true;
    }
    
    return false;
}

bool NodeManager::stopNode(const std::string& tenantId) {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    return stopContainer(tenantId);
}

bool NodeManager::startNode(const std::string& tenantId, int port, int memoryLimitMb) {
    return ensureNodeRunning(tenantId, port, memoryLimitMb);
}

std::shared_ptr<RedisNode> NodeManager::getNode(const std::string& tenantId) const {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    auto it = nodes_.find(tenantId);
    if (it != nodes_.end()) {
        return it->second;
    }
    return nullptr;
}

bool NodeManager::isContainerRunning(const std::string& containerId) {
    if (containerId.empty()) {
        return false;
    }
    
    std::string cmd = "docker inspect -f {{.State.Running}} " + containerId;
    cmd += NULL_REDIRECT;
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return false;
    }
    
    char buffer[16];
    std::string result;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result = buffer;
    }
    pclose(pipe);
    
    return result.find("true") != std::string::npos;
}

void NodeManager::cleanupIdleNodes() {
    std::vector<std::string> toStop;
    
    for (const auto& pair : nodes_) {
        const auto& node = pair.second;
        if (node->isRunning_ && node->getIdleSeconds() > IDLE_TIMEOUT_SECONDS) {
            toStop.push_back(pair.first);
        }
    }
    
    for (const auto& tenantId : toStop) {
        std::cout << "[NodeManager] Cleaning up idle node: " << tenantId 
                  << " (idle for " << nodes_[tenantId]->getIdleSeconds() << " seconds)" << std::endl;
        stopContainer(tenantId);
    }
}

void NodeManager::cleanupLoop() {
    while (!stopCleanup_) {
        std::this_thread::sleep_for(std::chrono::seconds(60));
        
        std::lock_guard<std::mutex> lock(nodesMutex_);
        cleanupIdleNodes();
    }
}

int NodeManager::getRunningCount() const {
    int count = 0;
    for (const auto& pair : nodes_) {
        if (pair.second->isRunning_) {
            count++;
        }
    }
    return count;
}

int NodeManager::getTotalCount() const {
    return static_cast<int>(nodes_.size());
}

std::vector<std::string> NodeManager::listNodes() const {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    std::vector<std::string> result;
    
    for (const auto& pair : nodes_) {
        std::ostringstream info;
        info << pair.first << " (running: " 
             << (pair.second->isRunning_ ? "yes" : "no") 
             << ", port: " << pair.second->port_ << ")";
        result.push_back(info.str());
    }
    
    return result;
}