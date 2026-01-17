#ifndef ERROR_DETECTOR_H
#define ERROR_DETECTOR_H

#include <string>
#include <map>

class ErrorDetector {
public:
    // Helper to detect if a reading contains an error
    // Currently detects DS18B20 sensors with temperature/value = 85 (error condition)
    static bool isErrorReading(const std::map<std::string, std::string>& reading);
};

#endif // ERROR_DETECTOR_H
