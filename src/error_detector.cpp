#include "error_detector.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include <cstring>

// Static member initialization
std::vector<ErrorDefinition> ErrorDetector::errorDefinitions;
bool ErrorDetector::definitionsLoaded = false;

namespace {
    // Case-insensitive string comparison
    bool equalsIgnoreCase(const std::string& a, const std::string& b) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(a[i])) != 
                std::tolower(static_cast<unsigned char>(b[i]))) {
                return false;
            }
        }
        return true;
    }
    
    // Trim whitespace from string
    std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }
    
    // Built-in fallback definitions (used if config file not found)
    std::vector<ErrorDefinition> getBuiltinDefinitions() {
        return {
            {"ds18b20", "value", "85", "DS18B20 communication error (value=85)"},
            {"ds18b20", "value", "-127", "DS18B20 disconnected or power-on reset (value=-127)"},
            {"ds18b20", "temperature", "85", "DS18B20 communication error (temperature=85)"},
            {"ds18b20", "temperature", "-127", "DS18B20 disconnected or power-on reset (temperature=-127)"}
        };
    }
}

// Load definitions from a single .errors file
// File format: field:value:description (sensor name derived from filename)
void loadFromFile(const std::string& filepath, const std::string& sensorName,
                  std::vector<ErrorDefinition>& definitions) {
    std::ifstream file(filepath);
    if (!file) return;
    
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        // Parse: field:value:description
        std::istringstream iss(line);
        std::string field, value, description;
        
        if (std::getline(iss, field, ':') &&
            std::getline(iss, value, ':') &&
            std::getline(iss, description)) {
            
            ErrorDefinition def;
            def.sensor = sensorName;
            def.field = trim(field);
            def.value = trim(value);
            def.description = trim(description);
            
            if (!def.field.empty() && !def.value.empty()) {
                definitions.push_back(def);
            }
        }
    }
}

// Extract sensor name from filename (e.g., "ds18b20.errors" -> "ds18b20")
std::string getSensorNameFromFilename(const std::string& filename) {
    size_t dotPos = filename.rfind('.');
    if (dotPos != std::string::npos) {
        return filename.substr(0, dotPos);
    }
    return filename;
}

void ErrorDetector::loadErrorDefinitions(const std::string& configDir) {
    errorDefinitions.clear();
    
    DIR* dir = opendir(configDir.c_str());
    if (!dir) {
        // Fall back to built-in definitions
        errorDefinitions = getBuiltinDefinitions();
        definitionsLoaded = true;
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        
        // Look for .errors files
        if (filename.length() > 7 && 
            filename.substr(filename.length() - 7) == ".errors") {
            
            std::string sensorName = getSensorNameFromFilename(filename);
            std::string filepath = configDir + "/" + filename;
            loadFromFile(filepath, sensorName, errorDefinitions);
        }
    }
    closedir(dir);
    
    // If no definitions loaded from files, use built-in
    if (errorDefinitions.empty()) {
        errorDefinitions = getBuiltinDefinitions();
    }
    
    definitionsLoaded = true;
}

void ErrorDetector::ensureLoaded() {
    if (!definitionsLoaded) {
        loadErrorDefinitions();
    }
}

bool ErrorDetector::isErrorReading(const std::map<std::string, std::string>& reading) {
    return !getErrorDescription(reading).empty();
}

std::string ErrorDetector::getErrorDescription(const std::map<std::string, std::string>& reading) {
    ensureLoaded();
    
    // Get the sensor type
    auto sensorIt = reading.find("sensor");
    if (sensorIt == reading.end()) {
        return "";
    }
    const std::string& sensorName = sensorIt->second;
    
    // Check each error definition
    for (const auto& def : errorDefinitions) {
        // Check if sensor matches (case-insensitive)
        if (!equalsIgnoreCase(sensorName, def.sensor)) {
            continue;
        }
        
        // Check if the specified field exists and has the error value
        auto fieldIt = reading.find(def.field);
        if (fieldIt != reading.end() && fieldIt->second == def.value) {
            return def.description;
        }
    }
    
    return "";
}
