#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>

class EnvLoader {
private:
    static std::unordered_map<std::string, std::string> env_vars;

public:
   
    static bool load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
       
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
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
        return true;
    }

   
    static std::string get(const std::string& key, const std::string& defaultValue = "") {

        auto it = env_vars.find(key);
        if (it != env_vars.end()) {
            return it->second;
        }
        
        
        const char* val = std::getenv(key.c_str());
        if (val != nullptr) {
            return std::string(val);
        }
        
        return defaultValue;
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
};

#endif 