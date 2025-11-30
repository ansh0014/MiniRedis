#include <drogon/drogon.h>
#include <iostream>

int main() {
  
    std::cout << "  MiniRedis Backend API v1.0\n";
    std::cout << "  Multi-Tenant Management\n";
    

    // Load config from file
    drogon::app().loadConfigFile("../config.json");
    
    std::cout << "[Backend] Starting HTTP server...\n";
    std::cout << "[Backend] Listening on port 5500\n\n";
    
    // Run the application
    drogon::app().run();
    
    return 0;
}