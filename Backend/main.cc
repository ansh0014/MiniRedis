#include <drogon/drogon.h>
#include <iostream>
#include "../config/config.h"

int main() {
    std::cout << "========================================\n";
    std::cout << "  MiniRedis Backend API v2.0\n";
    std::cout << "  Multi-Tenant Management\n";
    std::cout << "========================================\n\n";

    try {
        // Load .env file - go up 3 levels from build/Release/ to root
        std::cout << "[Backend] Loading environment...\n";
        if (!EnvLoader::load("../../../.env")) {  // Changed from ../.env to ../../../.env
            std::cerr << "[ERROR] Failed to load .env file\n";
            std::cerr << "[ERROR] Looking for: ../../../.env from " << std::filesystem::current_path() << "\n";
            return 1;
        }
        std::cout << "[Backend]  Environment loaded\n\n";
        
        // Get configuration
        std::string dbHost = EnvLoader::get("DB_HOST", "localhost");
        int dbPort = EnvLoader::getInt("DB_PORT", 5432);
        std::string dbName = EnvLoader::get("DB_NAME", "miniredis");
        std::string dbUser = EnvLoader::get("DB_USER", "postgres");
        std::string dbPassword = EnvLoader::get("DB_PASSWORD");
        int backendPort = EnvLoader::getInt("BACKEND_PORT", 5500);
        std::string logLevel = EnvLoader::get("LOG_LEVEL", "INFO");

        if (dbPassword.empty()) {
            std::cerr << "[ERROR] DB_PASSWORD not set in .env file\n";
            return 1;
        }

        // Configure database
        std::cout << "[Backend] Connecting to database...\n";
        std::cout << "  Host: " << dbHost << ":" << dbPort << "\n";
        std::cout << "  Database: " << dbName << "\n";
        std::cout << "  User: " << dbUser << "\n";

        drogon::app().createDbClient(
            "postgresql",
            dbHost,
            dbPort,
            dbName,
            dbUser,
            dbPassword,
            1,
            "",
            "default",
            false,
            ""
        );

        std::cout << "[Backend] ✓ Database connected\n\n";

        // Configure logging
        if (logLevel == "DEBUG") {
            drogon::app().setLogLevel(trantor::Logger::kDebug);
        } else if (logLevel == "INFO") {
            drogon::app().setLogLevel(trantor::Logger::kInfo);
        } else if (logLevel == "WARN") {
            drogon::app().setLogLevel(trantor::Logger::kWarn);
        } else if (logLevel == "ERROR") {
            drogon::app().setLogLevel(trantor::Logger::kError);
        }

        // Configure server
        drogon::app().addListener("0.0.0.0", backendPort);
        drogon::app().setThreadNum(4);

        std::cout << "[Backend] Starting HTTP server...\n";
        std::cout << "[Backend] Listening on http://0.0.0.0:" << backendPort << "\n";
        std::cout << "[Backend] Press Ctrl+C to stop\n\n";
        
        std::cout << "API Endpoints:\n";
        std::cout << "  POST   /api/tenants          - Create tenant\n";
        std::cout << "  GET    /api/tenants/{id}     - Get tenant info\n";
        std::cout << "  POST   /api/apikeys          - Create API key\n";
        std::cout << "  GET    /api/verify?key=...   - Verify API key\n";
        std::cout << "  DELETE /api/apikeys/{key}    - Revoke API key\n\n";
        
        // Run
        drogon::app().run();
        
    } catch (const std::exception &e) {
        std::cerr << "\n[ERROR] " << e.what() << "\n\n";
        std::cerr << "Check:\n";
        std::cerr << "  - PostgreSQL: pg_isready -h localhost\n";
        std::cerr << "  - Redis: redis-cli ping\n";
        std::cerr << "  - .env file with DB_PASSWORD set\n\n";
        return 1;
    }

    return 0;
}