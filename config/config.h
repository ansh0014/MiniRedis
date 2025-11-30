#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>

class EnvLoader {
public:
    static bool load(const std::string& filename = ".env") {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "[ENV] Warning: " << filename << " not found\n";
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;

            // Find = separator
            size_t pos = line.find('=');
            if (pos == std::string::npos) continue;

            std::string key = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));

            // Remove quotes if present
            if (!value.empty() && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }

            env_vars[key] = value;
        }

        std::cout << "[ENV] Loaded " << env_vars.size() << " environment variables\n";
        return true;
    }

    static std::string get(const std::string& key, const std::string& defaultValue = "") {
        auto it = env_vars.find(key);
        return (it != env_vars.end()) ? it->second : defaultValue;
    }

    static int getInt(const std::string& key, int defaultValue = 0) {
        auto value = get(key);
        return value.empty() ? defaultValue : std::stoi(value);
    }

private:
    static std::unordered_map<std::string, std::string> env_vars;

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }
};

// Initialize static member
std::unordered_map<std::string, std::string> EnvLoader::env_vars;