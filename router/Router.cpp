#include "Router.h"
#include <iostream>
#include <sstream>
#include <drogon/drogon.h>
#include <curl/curl.h>

using namespace drogon;
using namespace sw::redis;

// Callback for CURL response
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

Router::Router(const std::string& backendUrl, int routerPort)
    : backendUrl_(backendUrl), routerPort_(routerPort) {}

void Router::start() {
    std::cout << "[Router] Starting on port " << routerPort_ << "...\n";
    
    // Setup HTTP server for Redis protocol
    app().registerHandler(
        "/redis",
        [this](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            // Get API key from header
            std::string apiKey = req->getHeader("X-API-Key");
            if (apiKey.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->setBody("{\"error\":\"Missing API key\"}");
                callback(resp);
                return;
            }

            // Verify API key and get tenant ID
            std::string tenantId = verifyApiKey(apiKey);
            if (tenantId.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k401Unauthorized);
                resp->setBody("{\"error\":\"Invalid API key\"}");
                callback(resp);
                return;
            }

            // Get Redis command from body
            std::string command = std::string(req->getBody());
            if (command.empty()) {
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k400BadRequest);
                resp->setBody("{\"error\":\"Empty command\"}");
                callback(resp);
                return;
            }

            // Forward to tenant's Redis node
            std::string result = forwardCommand(tenantId, command);
            
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k200OK);
            resp->setBody(result);
            callback(resp);
        },
        {Post}
    );

    app().addListener("0.0.0.0", routerPort_);
    app().setThreadNum(4);
    
    std::cout << "[Router] Listening on http://0.0.0.0:" << routerPort_ << "\n";
    std::cout << "[Router] Backend API: " << backendUrl_ << "\n\n";
    
    app().run();
}

std::string Router::verifyApiKey(const std::string& apiKey) {
    // Check cache first
    auto it = apiKeyCache_.find(apiKey);
    if (it != apiKeyCache_.end()) {
        return it->second;
    }

    // Call backend API
    std::string tenantId = callBackendVerify(apiKey);
    if (!tenantId.empty()) {
        apiKeyCache_[apiKey] = tenantId;
    }
    
    return tenantId;
}

std::string Router::callBackendVerify(const std::string& apiKey) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[Router] Failed to init CURL\n";
        return "";
    }

    std::string url = backendUrl_ + "/api/verify?key=" + apiKey;
    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "[Router] Backend API call failed: " << curl_easy_strerror(res) << "\n";
        return "";
    }

    // Parse JSON response: {"tenant_id":"..."}
    size_t pos = response.find("\"tenant_id\":\"");
    if (pos != std::string::npos) {
        pos += 13; // Length of "tenant_id":""
        size_t end = response.find("\"", pos);
        if (end != std::string::npos) {
            return response.substr(pos, end - pos);
        }
    }

    return "";
}

std::shared_ptr<Redis> Router::getRedisConnection(const std::string& tenantId, int port) {
    auto it = redisConnections_.find(tenantId);
    if (it != redisConnections_.end()) {
        return it->second;
    }

    // Create new connection
    ConnectionOptions opts;
    opts.host = "127.0.0.1";
    opts.port = port;
    opts.socket_timeout = std::chrono::milliseconds(100);

    try {
        auto redis = std::make_shared<Redis>(opts);
        redisConnections_[tenantId] = redis;
        
        std::cout << "[Router] Connected to Redis node for tenant " << tenantId << " on port " << port << "\n";
        
        return redis;
    } catch (const std::exception& e) {
        std::cerr << "[Router] Failed to connect to Redis: " << e.what() << "\n";
        return nullptr;
    }
}

std::string Router::forwardCommand(const std::string& tenantId, const std::string& command) {
    try {
        // TODO: Get port from backend API /api/tenants/{tenantId}
        // For now, use default port 6379
        int port = 6379;
        
        auto redis = getRedisConnection(tenantId, port);
        if (!redis) {
            return "-ERR failed to connect to Redis node\r\n";
        }
        
        // Parse command (simple implementation)
        // Format: "SET key value" or "GET key"
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "SET") {
            std::string key, value;
            iss >> key;
            // Read the rest as value (may contain spaces)
            std::getline(iss, value);
            // Trim leading space
            if (!value.empty() && value[0] == ' ') {
                value = value.substr(1);
            }
            redis->set(key, value);
            return "+OK\r\n";
        } else if (cmd == "GET") {
            std::string key;
            iss >> key;
            auto val = redis->get(key);
            if (val) {
                return "$" + std::to_string(val->size()) + "\r\n" + *val + "\r\n";
            }
            return "$-1\r\n";
        } else if (cmd == "DEL") {
            std::string key;
            iss >> key;
            long long count = redis->del(key);
            return ":" + std::to_string(count) + "\r\n";
        } else if (cmd == "PING") {
            return "+PONG\r\n";
        }
        
        return "-ERR unknown command '" + cmd + "'\r\n";
        
    } catch (const std::exception& e) {
        std::cerr << "[Router] Error forwarding command: " << e.what() << "\n";
        return "-ERR " + std::string(e.what()) + "\r\n";
    }
}