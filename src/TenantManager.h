#ifndef TENANT_MANAGER_H
#define TENANT_MANAGER_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <sstream>
#include <iostream>
#include <atomic>
#include <iomanip>
#include <algorithm>

using namespace std;

namespace Config {
    const size_t DEFAULT_MEMORY_LIMIT_MB = 40;
    const size_t MB_TO_BYTES = 1024 * 1024;
}


class TenantConfig {
public:
    string id;
    string name;
    size_t memoryLimitBytes;
    int nodePort;
    atomic<size_t> currentMemoryUsage;

    TenantConfig() : memoryLimitBytes(0), nodePort(0), currentMemoryUsage(0) {}
    
    TenantConfig(const string& tenantId, const string& tenantName, 
                 size_t memLimitMB, int port)
        : id(tenantId), name(tenantName), 
          memoryLimitBytes(memLimitMB * Config::MB_TO_BYTES), 
          nodePort(port), currentMemoryUsage(0) {}


    TenantConfig(const TenantConfig& other)
        : id(other.id), name(other.name),
          memoryLimitBytes(other.memoryLimitBytes),
          nodePort(other.nodePort),
          currentMemoryUsage(other.currentMemoryUsage.load()) {}


    TenantConfig& operator=(const TenantConfig& other) {
        if (this != &other) {
            id = other.id;
            name = other.name;
            memoryLimitBytes = other.memoryLimitBytes;
            nodePort = other.nodePort;
            currentMemoryUsage.store(other.currentMemoryUsage.load());
        }
        return *this;
    }

    
    bool canAllocate(size_t bytes) const {
        return (currentMemoryUsage.load() + bytes) <= memoryLimitBytes;
    }

 
    void allocate(size_t bytes) {
        currentMemoryUsage.fetch_add(bytes);
    }


    void deallocate(size_t bytes) {
        currentMemoryUsage.fetch_sub(bytes);
    }

 
    double getUsagePercent() const {
        if (memoryLimitBytes == 0) return 0.0;
        return (double)currentMemoryUsage.load() / (double)memoryLimitBytes * 100.0;
    }

  
    size_t getAvailableMemory() const {
        size_t used = currentMemoryUsage.load();
        return (used < memoryLimitBytes) ? (memoryLimitBytes - used) : 0;
    }
};


class TenantManager {
private:
    unordered_map<string, TenantConfig> tenants;
    unordered_map<string, string> apiKeyToTenant;
    mutable mutex mtx;
    string apiKeysFile;

    // Trim whitespace helper
    static string trim(const string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == string::npos) return "";
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

public:
 
    explicit TenantManager(const string& apiKeysPath = "config/apikeys.txt") 
        : apiKeysFile(apiKeysPath) {
        cout << "[TenantMgr] Initialized with API keys file: " << apiKeysFile << "\n";
    }


    ~TenantManager() {
        cout << "[TenantMgr] Shutting down\n";
    }

    bool loadAPIKeys() {
        lock_guard<mutex> lock(mtx);
        apiKeyToTenant.clear();
        
        ifstream f(apiKeysFile);
        if (!f.is_open()) {
            cerr << "[TenantMgr] ERROR: Could not open " << apiKeysFile << "\n";
            return false;
        }
        
        string line;
        int count = 0;
        int lineNum = 0;
        
        while (getline(f, line)) {
            lineNum++;
            line = trim(line);
            
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;
            
            size_t colon = line.find(':');
            if (colon == string::npos) {
                cerr << "[TenantMgr] WARNING: Invalid format at line " << lineNum 
                     << " (expected 'apikey:tenantId')\n";
                continue;
            }
            
            string apiKey = trim(line.substr(0, colon));
            string tenantId = trim(line.substr(colon + 1));
            
            if (apiKey.empty() || tenantId.empty()) {
                cerr << "[TenantMgr] WARNING: Empty key or tenant at line " << lineNum << "\n";
                continue;
            }
            
            apiKeyToTenant[apiKey] = tenantId;
            count++;
        }
        
        f.close();
        cout << "[TenantMgr] Successfully loaded " << count << " API key(s)\n";
        return count > 0;
    }
    void addTenant(const string& id, const string& name, size_t memLimitMB, int port) {
        lock_guard<mutex> lock(mtx);
        
        TenantConfig cfg(id, name, memLimitMB, port);
        tenants[id] = cfg;
        
        cout << "[TenantMgr] Registered tenant: " << id << " (" << name << ")\n"
             << "  - Node port: " << port << "\n"
             << "  - Memory limit: " << memLimitMB << " MB (" 
             << (memLimitMB * Config::MB_TO_BYTES) << " bytes)\n";
    }

 
    void addTenant(const string& id, const string& name, int port) {
        addTenant(id, name, Config::DEFAULT_MEMORY_LIMIT_MB, port);
    }


    string authenticate(const string& apiKey) const {
        lock_guard<mutex> lock(mtx);
        auto it = apiKeyToTenant.find(apiKey);
        if (it != apiKeyToTenant.end()) {
            return it->second;
        }
        return "";
    }


    bool getTenantConfig(const string& tenantId, TenantConfig& cfg) const {
        lock_guard<mutex> lock(mtx);
        auto it = tenants.find(tenantId);
        if (it != tenants.end()) {
            cfg = it->second;
            return true;
        }
        return false;
    }

   
    TenantConfig* getTenantConfigPtr(const string& tenantId) {
        lock_guard<mutex> lock(mtx);
        auto it = tenants.find(tenantId);
        if (it != tenants.end()) {
            return &(it->second);
        }
        return nullptr;
    }


    int getNodePort(const string& tenantId) const {
        lock_guard<mutex> lock(mtx);
        auto it = tenants.find(tenantId);
        return (it != tenants.end()) ? it->second.nodePort : -1;
    }

    size_t getMemoryLimit(const string& tenantId) const {
        lock_guard<mutex> lock(mtx);
        auto it = tenants.find(tenantId);
        return (it != tenants.end()) ? it->second.memoryLimitBytes : 0;
    }


    bool canAllocate(const string& tenantId, size_t bytes) const {
        lock_guard<mutex> lock(mtx);
        auto it = tenants.find(tenantId);
        return (it != tenants.end()) ? it->second.canAllocate(bytes) : false;
    }

    bool allocateMemory(const string& tenantId, size_t bytes) {
        lock_guard<mutex> lock(mtx);
        auto it = tenants.find(tenantId);
        if (it != tenants.end() && it->second.canAllocate(bytes)) {
            it->second.allocate(bytes);
            return true;
        }
        return false;
    }


    void deallocateMemory(const string& tenantId, size_t bytes) {
        lock_guard<mutex> lock(mtx);
        auto it = tenants.find(tenantId);
        if (it != tenants.end()) {
            it->second.deallocate(bytes);
        }
    }


    string getTenantStats(const string& tenantId) const {
        lock_guard<mutex> lock(mtx);
        auto it = tenants.find(tenantId);
        if (it == tenants.end()) {
            return "Tenant not found";
        }

        const TenantConfig& cfg = it->second;
        ostringstream oss;
        oss << "Tenant: " << cfg.id << " (" << cfg.name << ")\n"
            << "Port: " << cfg.nodePort << "\n"
            << "Memory: " << cfg.currentMemoryUsage.load() << " / " 
            << cfg.memoryLimitBytes << " bytes\n";
        oss.setf(std::ios::fixed, std::ios::floatfield);
        oss.precision(2);
        oss << "Usage: " << cfg.getUsagePercent() << "%\n"
            << "Available: " << cfg.getAvailableMemory() << " bytes\n";
        return oss.str();
    }


    string getAllStats() const {
        lock_guard<mutex> lock(mtx);
        ostringstream oss;
        oss << "=== Tenant Statistics ===\n";
        for (const auto& pair : tenants) {
            const TenantConfig& cfg = pair.second;
            oss << "\nTenant: " << cfg.id << " (" << cfg.name << ")\n"
                << "  Port: " << cfg.nodePort << "\n"
                << "  Memory: " << cfg.currentMemoryUsage.load() << " / " 
                << cfg.memoryLimitBytes << " bytes (";
            oss.setf(std::ios::fixed, std::ios::floatfield);
            oss.precision(2);
            oss << cfg.getUsagePercent() << "%)\n";
        }
        return oss.str();
    }


    bool tenantExists(const string& tenantId) const {
        lock_guard<mutex> lock(mtx);
        return tenants.find(tenantId) != tenants.end();
    }

    // Get tenant count
    size_t getTenantCount() const {
        lock_guard<mutex> lock(mtx);
        return tenants.size();
    }
};

#endif /