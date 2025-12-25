#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>

class EnvLoader {
public:
    static bool load(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[WARN] .env file not found: " << filepath << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;
            
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t\r\n"));
                key.erase(key.find_last_not_of(" \t\r\n") + 1);
                value.erase(0, value.find_first_not_of(" \t\r\n"));
                value.erase(value.find_last_not_of(" \t\r\n") + 1);
                
                env_vars[key] = value;
            }
        }
        file.close();
        return true;
    }

    static std::string get(const std::string& key, const std::string& defaultValue = "") {
        auto it = env_vars.find(key);
        if (it != env_vars.end()) {
            return it->second;
        }
        // Fallback to system environment
        const char* val = std::getenv(key.c_str());
        return val ? std::string(val) : defaultValue;
    }

    static int getInt(const std::string& key, int defaultValue = 0) {
        std::string val = get(key);
        if (val.empty()) return defaultValue;
        try {
            return std::stoi(val);
        } catch (...) {
            return defaultValue;
        }
    }

    static bool getBool(const std::string& key, bool defaultValue = false) {
        std::string val = get(key);
        if (val.empty()) return defaultValue;
        return (val == "true" || val == "1" || val == "yes");
    }

private:
    static std::unordered_map<std::string, std::string> env_vars;
};