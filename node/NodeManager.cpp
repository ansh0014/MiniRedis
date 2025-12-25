#include "NodeManager.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>

// ============= RedisNode Implementation =============

RedisNode::RedisNode(const std::string& tenantId, int port, int memoryLimitMb)
    : tenantId_(tenantId), 
      port_(port),
      memoryLimitBytes_(memoryLimitMb * 1024 * 1024),
      running_(false) {
}

RedisNode::~RedisNode() {
    stop();
}

void RedisNode::start() {
    if (running_) return;
    
    running_ = true;
    
    // Start TTL sweeper thread
    sweeperThread_ = std::thread(&RedisNode::ttlSweeperLoop, this);
    
    std::cout << "[Node] Started node for tenant: " << tenantId_ 
              << " on port " << port_ 
              << " (Memory limit: " << (memoryLimitBytes_ / 1024 / 1024) << "MB)\n";
}

void RedisNode::stop() {
    if (!running_) return;
    
    running_ = false;
    
    if (sweeperThread_.joinable()) {
        sweeperThread_.join();
    }
    
    std::cout << "[Node] Stopped node for tenant: " << tenantId_ << "\n";
}

void RedisNode::ttlSweeperLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::lock_guard<std::mutex> lock(storageMutex_);
        
        // Remove expired keys
        for (auto it = storage_.begin(); it != storage_.end();) {
            if (it->second.isExpired()) {
                it = storage_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

std::string RedisNode::set(const std::string& key, const std::string& value, int ttl) {
    std::lock_guard<std::mutex> lock(storageMutex_);
    
    size_t newSize = key.size() + value.size() + sizeof(KVEntry);
    
    if (!checkMemoryLimit(newSize)) {
        evictLRU();
        if (!checkMemoryLimit(newSize)) {
            return "-ERR OOM command not allowed when used memory > 'maxmemory'\r\n";
        }
    }
    
    if (ttl > 0) {
        storage_[key] = KVEntry(value, ttl);
    } else {
        storage_[key] = KVEntry(value);
    }
    
    return "+OK\r\n";
}

std::string RedisNode::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(storageMutex_);
    
    auto it = storage_.find(key);
    if (it == storage_.end()) {
        return "$-1\r\n"; // Null bulk reply
    }
    
    if (it->second.isExpired()) {
        storage_.erase(it);
        return "$-1\r\n";
    }
    
    const std::string& value = it->second.value;
    return "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
}

std::string RedisNode::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(storageMutex_);
    
    auto count = storage_.erase(key);
    return ":" + std::to_string(count) + "\r\n";
}

std::string RedisNode::exists(const std::string& key) {
    std::lock_guard<std::mutex> lock(storageMutex_);
    
    auto it = storage_.find(key);
    if (it != storage_.end() && !it->second.isExpired()) {
        return ":1\r\n";
    }
    return ":0\r\n";
}

std::string RedisNode::keys(const std::string& pattern) {
    std::lock_guard<std::mutex> lock(storageMutex_);
    
    std::vector<std::string> matchedKeys;
    
    for (const auto& [key, entry] : storage_) {
        if (!entry.isExpired()) {
            // Simple pattern matching (* = all)
            if (pattern == "*") {
                matchedKeys.push_back(key);
            }
        }
    }
    
    std::string result = "*" + std::to_string(matchedKeys.size()) + "\r\n";
    for (const auto& key : matchedKeys) {
        result += "$" + std::to_string(key.size()) + "\r\n" + key + "\r\n";
    }
    
    return result;
}

std::string RedisNode::flushall() {
    std::lock_guard<std::mutex> lock(storageMutex_);
    storage_.clear();
    return "+OK\r\n";
}

std::string RedisNode::ping() {
    return "+PONG\r\n";
}

size_t RedisNode::getMemoryUsage() const {
    std::lock_guard<std::mutex> lock(storageMutex_);
    
    size_t total = 0;
    for (const auto& [key, entry] : storage_) {
        total += key.size() + entry.value.size() + sizeof(KVEntry);
    }
    return total;
}

size_t RedisNode::getKeyCount() const {
    std::lock_guard<std::mutex> lock(storageMutex_);
    return storage_.size();
}

bool RedisNode::checkMemoryLimit(size_t additionalBytes) {
    return (getMemoryUsage() + additionalBytes) <= memoryLimitBytes_;
}

void RedisNode::evictLRU() {
    // Simple eviction: remove first key (LRU would be more complex)
    if (!storage_.empty()) {
        std::cout << "[Node] Memory limit reached, evicting key: " << storage_.begin()->first << "\n";
        storage_.erase(storage_.begin());
    }
}

// ============= NodeManager Implementation =============

NodeManager::NodeManager() {
    std::cout << "[NodeManager] Initialized\n";
}

NodeManager::~NodeManager() {
    stopAllNodes();
}

bool NodeManager::startNode(const std::string& tenantId, int port, int memoryLimitMb) {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    
    if (nodes_.find(tenantId) != nodes_.end()) {
        std::cout << "[NodeManager] Node already exists for tenant: " << tenantId << "\n";
        return true;
    }
    
    auto node = std::make_shared<RedisNode>(tenantId, port, memoryLimitMb);
    node->start();
    
    nodes_[tenantId] = node;
    return true;
}

bool NodeManager::stopNode(const std::string& tenantId) {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    
    auto it = nodes_.find(tenantId);
    if (it == nodes_.end()) {
        std::cerr << "[NodeManager] Node not found: " << tenantId << "\n";
        return false;
    }
    
    it->second->stop();
    nodes_.erase(it);
    return true;
}

std::shared_ptr<RedisNode> NodeManager::getNode(const std::string& tenantId) {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    
    auto it = nodes_.find(tenantId);
    if (it != nodes_.end()) {
        return it->second;
    }
    return nullptr;
}

std::string NodeManager::executeCommand(const std::string& tenantId, const std::string& command) {
    auto node = getNode(tenantId);
    if (!node) {
        return "-ERR tenant not found\r\n";
    }
    
    // Parse Redis command
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;
    
    // Convert to uppercase
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    
    if (cmd == "SET") {
        std::string key, value;
        int ttl = 0;
        iss >> key >> value;
        
        // Check for EX (TTL in seconds)
        std::string ttlFlag;
        if (iss >> ttlFlag) {
            std::transform(ttlFlag.begin(), ttlFlag.end(), ttlFlag.begin(), ::toupper);
            if (ttlFlag == "EX") {
                iss >> ttl;
            }
        }
        
        return node->set(key, value, ttl);
        
    } else if (cmd == "GET") {
        std::string key;
        iss >> key;
        return node->get(key);
        
    } else if (cmd == "DEL") {
        std::string key;
        iss >> key;
        return node->del(key);
        
    } else if (cmd == "EXISTS") {
        std::string key;
        iss >> key;
        return node->exists(key);
        
    } else if (cmd == "KEYS") {
        std::string pattern;
        iss >> pattern;
        return node->keys(pattern);
        
    } else if (cmd == "FLUSHALL") {
        return node->flushall();
        
    } else if (cmd == "PING") {
        return node->ping();
        
    } else if (cmd == "INFO") {
        std::string info = "# Memory\r\n";
        info += "used_memory:" + std::to_string(node->getMemoryUsage()) + "\r\n";
        info += "used_memory_human:" + std::to_string(node->getMemoryUsage() / 1024) + "K\r\n";
        info += "# Keyspace\r\n";
        info += "db0:keys=" + std::to_string(node->getKeyCount()) + "\r\n";
        return "$" + std::to_string(info.size()) + "\r\n" + info + "\r\n";
    }
    
    return "-ERR unknown command '" + cmd + "'\r\n";
}

std::vector<std::string> NodeManager::listNodes() {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    
    std::vector<std::string> result;
    for (const auto& [tenantId, node] : nodes_) {
        result.push_back(tenantId);
    }
    return result;
}

void NodeManager::stopAllNodes() {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    
    std::cout << "[NodeManager] Stopping all nodes...\n";
    for (auto& [tenantId, node] : nodes_) {
        node->stop();
    }
    nodes_.clear();
}