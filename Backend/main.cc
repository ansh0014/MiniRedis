#include <drogon/drogon.h>
#include <iostream>
#include "../config/config.h"

int main() {
    std::cout << "MiniRedis Backend API v2.0\n";
    std::cout << "Multi-Tenant Management System\n\n";

    try {
      
        std::cout << "[Backend] Loading configuration from environment variables\n";
        
        std::string dbHost = EnvLoader::get("DB_HOST", "localhost");
        int dbPort = EnvLoader::getInt("DB_PORT", 5432);
        std::string dbName = EnvLoader::get("DB_NAME", "miniredis");
        std::string dbUser = EnvLoader::get("DB_USER", "postgres");
        std::string dbPassword = EnvLoader::get("DB_PASSWORD");
        int backendPort = EnvLoader::getInt("BACKEND_PORT", 5500);
        std::string logLevel = EnvLoader::get("LOG_LEVEL", "INFO");

        if (dbPassword.empty()) {
            std::cerr << "[ERROR] DB_PASSWORD not set\n";
            return 1;
        }

        std::cout << "[Backend] Database: " << dbHost << ":" << dbPort << "/" << dbName << "\n";
        std::cout << "[Backend] User: " << dbUser << "\n";

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

        if (logLevel == "DEBUG") {
            drogon::app().setLogLevel(trantor::Logger::kDebug);
        } else if (logLevel == "INFO") {
            drogon::app().setLogLevel(trantor::Logger::kInfo);
        } else if (logLevel == "WARN") {
            drogon::app().setLogLevel(trantor::Logger::kWarn);
        } else if (logLevel == "ERROR") {
            drogon::app().setLogLevel(trantor::Logger::kError);
        }

        drogon::app().addListener("0.0.0.0", backendPort);
        drogon::app().setThreadNum(4);

        std::cout << "[Backend]  Server listening on port " << backendPort << "\n\n";
        
        drogon::app().run();
        
    } catch (const std::exception &e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }

    return 0;
}