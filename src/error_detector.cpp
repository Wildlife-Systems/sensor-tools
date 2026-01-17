#include "error_detector.h"
#include <algorithm>

bool ErrorDetector::isErrorReading(const std::map<std::string, std::string>& reading) {
    // Check for DS18B20 sensor error (temperature = 85)
    auto sensorIt = reading.find("sensor");
    auto valueIt = reading.find("value");
    auto tempIt = reading.find("temperature");
    
    // Check if sensor field is ds18b20 (case insensitive check)
    bool isDS18B20 = false;
    if (sensorIt != reading.end()) {
        std::string sensor = sensorIt->second;
        std::transform(sensor.begin(), sensor.end(), sensor.begin(), ::tolower);
        isDS18B20 = (sensor == "ds18b20");
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
