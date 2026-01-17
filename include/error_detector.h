#ifndef ERROR_DETECTOR_H
#define ERROR_DETECTOR_H

#include <string>
#include <map>

class ErrorDetector {
public:
    // Helper to detect if a reading contains an error
    // Currently detects DS18B20 sensors with:
    //   - temperature/value = 85 (communication error)
    //   - temperature/value = -127 (power-on reset/disconnected sensor)
    static bool isErrorReading(const std::map<std::string, std::string>& reading);
    
    // Get a description of the error, or empty string if no error
    static std::string getErrorDescription(const std::map<std::string, std::string>& reading);
};

#endif // ERROR_DETECTOR_H
