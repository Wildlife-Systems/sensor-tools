#include "error_detector.h"
#include <algorithm>

bool ErrorDetector::isErrorReading(const std::map<std::string, std::string>& reading) {
    // Check for DS18B20 sensor error (temperature = 85)
    auto typeIt = reading.find("type");
    auto valueIt = reading.find("value");
    auto tempIt = reading.find("temperature");
    
    // Check if type is ds18b20 or DS18B20 (case insensitive check)
    bool isDS18B20 = false;
    if (typeIt != reading.end()) {
        std::string type = typeIt->second;
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);
        isDS18B20 = (type == "ds18b20");
    }
    
    if (isDS18B20) {
        // Check value field
        if (valueIt != reading.end() && valueIt->second == "85") {
            return true;
        }
        // Also check temperature field as fallback
        if (tempIt != reading.end() && tempIt->second == "85") {
            return true;
        }
    }
    
    return false;
}
