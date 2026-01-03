#include <drogon/drogon.h>
#include <iostream>
#include "NodeManager.h"
#include "../config/config.h"

using namespace drogon;

NodeManager* nodeManager = nullptr;

int main() {
    std::cout << "========================================\n";
    std::cout << "  MiniRedis Node Manager v1.0\n";
    std::cout << "  In-Memory KV Store Service\n";
    std::cout << "========================================\n\n";

    try {
        std::cout << "[NodeManager] Loading environment...\n";
        if (!EnvLoader::load("../../../.env")) {
            std::cerr << "[ERROR] Failed to load .env file\n";
            return 1;
        }
        std::cout << "[NodeManager] ✓ Environment loaded\n\n";

        nodeManager = new NodeManager();

        // Endpoint: Start node (called by Backend)
        app().registerHandler(
            "/node/start",
            [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
                auto json = req->getJsonObject();
                
                // Log received data
                std::cout << "[NodeManager] Received start request:" << std::endl;
                std::cout << "  Raw JSON: " << json->toStyledString() << std::endl;
                
                if (!json->isMember("tenant_id") || !json->isMember("port")) {
                    std::cout << "[NodeManager] ❌ Missing required fields!" << std::endl;
                    Json::Value error;
                    error["error"] = "Missing tenant_id or port";
                    auto resp = HttpResponse::newHttpJsonResponse(error);
                    resp->setStatusCode(k400BadRequest);
                    callback(resp);
                    return;
                }
                
                std::string tenantId = (*json)["tenant_id"].asString();
                int port = (*json)["port"].asInt();
                int memoryMb = (*json).get("memory_limit_mb", 40).asInt();

                std::cout << "[NodeManager] Starting node:" << std::endl;
                std::cout << "  Tenant ID: " << tenantId << std::endl;
                std::cout << "  Port: " << port << std::endl;
                std::cout << "  Memory: " << memoryMb << "MB" << std::endl;

                bool success = nodeManager->startNode(tenantId, port, memoryMb);

                Json::Value response;
                response["success"] = success;
                response["tenant_id"] = tenantId;
                response["port"] = port;
                response["memory_limit_mb"] = memoryMb;

                auto resp = HttpResponse::newHttpJsonResponse(response);
                resp->setStatusCode(success ? k200OK : k500InternalServerError);
                
                std::cout << "[NodeManager] " << (success ? "✅ Node started" : "❌ Failed to start node") << std::endl;
                
                callback(resp);
            },
            {Post}
        );

        // Endpoint: Execute command (called by Router)
        app().registerHandler(
            "/node/execute",
            [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
                auto json = req->getJsonObject();
                
                std::cout << "[NodeManager] Execute request:" << std::endl;
                std::cout << "  Raw JSON: " << json->toStyledString() << std::endl;
                
                std::string tenantId = (*json)["tenant_id"].asString();
                std::string command = (*json)["command"].asString();

                std::cout << "[NodeManager] Executing for tenant: " << tenantId << std::endl;
                std::cout << "[NodeManager] Command: " << command << std::endl;

                std::string result = nodeManager->executeCommand(tenantId, command);

                std::cout << "[NodeManager] Result: " << result << std::endl;

                auto resp = HttpResponse::newHttpResponse();
                resp->setBody(result);
                resp->setContentTypeCode(CT_TEXT_PLAIN);
                callback(resp);
            },
            {Post}
        );

        // Endpoint: Stop node
        app().registerHandler(
            "/node/stop",
            [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
                auto json = req->getJsonObject();
                std::string tenantId = (*json)["tenant_id"].asString();

                bool success = nodeManager->stopNode(tenantId);

                Json::Value response;
                response["success"] = success;

                auto resp = HttpResponse::newHttpJsonResponse(response);
                callback(resp);
            },
            {Post}
        );

        // Endpoint: List nodes - UPDATED TO RETURN FULL INFO
        app().registerHandler(
            "/node/list",
            [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
                auto tenantIds = nodeManager->listNodes();
                
                Json::Value response(Json::arrayValue);
                
                for (const auto& tenantId : tenantIds) {
                    auto node = nodeManager->getNode(tenantId);
                    if (node) {
                        Json::Value nodeInfo;
                        nodeInfo["tenant_id"] = tenantId;
                        nodeInfo["port"] = node->getPort();
                        nodeInfo["status"] = node->isRunning() ? "running" : "stopped";
                        nodeInfo["memory_used"] = (int)node->getMemoryUsage();
                        nodeInfo["key_count"] = (int)node->getKeyCount();
                        
                        // Add ISO timestamp (current time for now)
                        auto now = std::chrono::system_clock::now();
                        auto time_t_now = std::chrono::system_clock::to_time_t(now);
                        char buffer[100];
                        std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S.000Z", 
                                    std::gmtime(&time_t_now));
                        nodeInfo["created_at"] = std::string(buffer);
                        
                        response.append(nodeInfo);
                    }
                }

                auto resp = HttpResponse::newHttpJsonResponse(response);
                callback(resp);
            },
            {Get}
        );

        int port = EnvLoader::getInt("NODE_MANAGER_PORT", 7000);
        app().addListener("0.0.0.0", port);
        app().setThreadNum(4);
        
        std::cout << "[NodeManager] HTTP API listening on http://0.0.0.0:" << port << "\n\n";
        std::cout << "API Endpoints:\n";
        std::cout << "  POST /node/start   - Create tenant node\n";
        std::cout << "  POST /node/execute - Execute Redis command\n";
        std::cout << "  POST /node/stop    - Stop node\n";
        std::cout << "  GET  /node/list    - List all nodes\n\n";

        app().run();

        delete nodeManager;

    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] " << e.what() << "\n\n";
        return 1;
    }

    return 0;
}