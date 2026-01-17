#include "error_detector.h"
#include <algorithm>
#include <cctype>

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
}

bool ErrorDetector::isErrorReading(const std::map<std::string, std::string>& reading) {
    return !getErrorDescription(reading).empty();
}

std::string ErrorDetector::getErrorDescription(const std::map<std::string, std::string>& reading) {
    auto sensorIt = reading.find("sensor");
    auto valueIt = reading.find("value");
    auto tempIt = reading.find("temperature");
    
    // Check if sensor field is ds18b20 (case insensitive check)
    bool isDS18B20 = (sensorIt != reading.end() && equalsIgnoreCase(sensorIt->second, "ds18b20"));
    
    if (isDS18B20) {
        // Check value field for error values
        if (valueIt != reading.end()) {
            const std::string& val = valueIt->second;
            if (val == "85") {
                return "DS18B20 communication error (value=85)";
            } else if (val == "-127") {
                return "DS18B20 disconnected or power-on reset (value=-127)";
            }
        }
        // Also check temperature field as fallback
        if (tempIt != reading.end()) {
            const std::string& temp = tempIt->second;
            if (temp == "85") {
                return "DS18B20 communication error (temperature=85)";
            } else if (temp == "-127") {
                return "DS18B20 disconnected or power-on reset (temperature=-127)";
            }
        }
    }
    
    return "";
}
