#include "error_detector.h"
#include <algorithm>

bool ErrorDetector::isErrorReading(const std::map<std::string, std::string>& reading) {
    // Check for DS18B20 sensor errors:
    //   - temperature = 85 (communication error)
    //   - temperature = -127 (power-on reset/disconnected sensor)
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
        // Check value field for error values
        if (valueIt != reading.end()) {
            const std::string& val = valueIt->second;
            if (val == "85" || val == "-127") {
                return true;
            }
        }
        // Also check temperature field as fallback
        if (tempIt != reading.end()) {
            const std::string& temp = tempIt->second;
            if (temp == "85" || temp == "-127") {
                return true;
            }
        }
    }
    
    return false;
}

std::string ErrorDetector::getErrorDescription(const std::map<std::string, std::string>& reading) {
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
