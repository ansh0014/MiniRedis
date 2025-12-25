#include <iostream>
#include "Router.h"
#include "../config/config.h"

int main() {
    std::cout << "========================================\n";
    std::cout << "  MiniRedis Router Service v1.0\n";
    std::cout << "  Request Forwarding & API Key Validation\n";
    std::cout << "========================================\n\n";

    try {
        // Load environment from .env file
      
        std::cout << "[Router] Loading environment...\n";
        if (!EnvLoader::load("../../../.env")) {  
            std::cerr << "[ERROR] Failed to load .env file\n";
            return 1;
        }
        std::cout << "[Router] ✓ Environment loaded\n\n";

        // Get configuration from .env
        std::string backendHost = EnvLoader::get("DB_HOST", "localhost");
        int backendPort = EnvLoader::getInt("BACKEND_PORT", 5500);
        int routerPort = EnvLoader::getInt("ROUTER_PORT", 6300);

        std::string backendUrl = "http://" + backendHost + ":" + std::to_string(backendPort);

        std::cout << "[Router] Configuration:\n";
        std::cout << "  Backend API: " << backendUrl << "\n";
        std::cout << "  Router Port: " << routerPort << "\n\n";

        // Create and start router
        Router router(backendUrl, routerPort);
        router.start();

    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] " << e.what() << "\n\n";
        return 1;
    }

    return 0;
}