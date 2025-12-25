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
                
                std::string tenantId = (*json)["tenant_id"].asString();
                int port = (*json)["port"].asInt();
                int memoryMb = (*json).get("memory_limit_mb", 40).asInt();

                bool success = nodeManager->startNode(tenantId, port, memoryMb);

                Json::Value response;
                response["success"] = success;
                response["tenant_id"] = tenantId;
                response["port"] = port;
                response["memory_limit_mb"] = memoryMb;

                auto resp = HttpResponse::newHttpJsonResponse(response);
                resp->setStatusCode(success ? k200OK : k500InternalServerError);
                callback(resp);
            },
            {Post}
        );

        // Endpoint: Execute command (called by Router)
        app().registerHandler(
            "/node/execute",
            [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
                auto json = req->getJsonObject();
                std::string tenantId = (*json)["tenant_id"].asString();
                std::string command = (*json)["command"].asString();

                std::string result = nodeManager->executeCommand(tenantId, command);

                auto resp = HttpResponse::newHttpResponse();
                resp->setBody(result);
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

        // Endpoint: List nodes
        app().registerHandler(
            "/node/list",
            [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
                auto nodes = nodeManager->listNodes();
                
                Json::Value response(Json::arrayValue);
                for (const auto& tenantId : nodes) {
                    response.append(tenantId);
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