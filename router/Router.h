#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <sw/redis++/redis++.h>
#include <drogon/HttpClient.h>

class Router {
public:
    Router(const std::string& backendUrl, int routerPort);
    ~Router() = default;

    // Start the router service
    void start();

    // Verify API key with backend
    std::string verifyApiKey(const std::string& apiKey);

    // Forward Redis command to tenant's node
    std::string forwardCommand(const std::string& tenantId, const std::string& command);

private:
    std::string backendUrl_;
    int routerPort_;
    
    // Cache: API Key -> Tenant ID
    std::unordered_map<std::string, std::string> apiKeyCache_;
    
    // Cache: Tenant ID -> Redis connection
    std::unordered_map<std::string, std::shared_ptr<sw::redis::Redis>> redisConnections_;
    
    // Get or create Redis connection for tenant
    std::shared_ptr<sw::redis::Redis> getRedisConnection(const std::string& tenantId, int port);
    
    // Call backend API to verify key
    std::string callBackendVerify(const std::string& apiKey);
};