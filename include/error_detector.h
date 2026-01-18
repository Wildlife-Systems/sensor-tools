#ifndef ERROR_DETECTOR_H
#define ERROR_DETECTOR_H

#include <string>
#include <map>
#include <vector>

// Structure to hold an error definition loaded from config
struct ErrorDefinition {
    std::string sensor;      // Sensor name (case-insensitive match)
    std::string field;       // Field to check (e.g., "value", "temperature")
    std::string value;       // Error value to match
    std::string description; // Human-readable error description
};

class ErrorDetector {
public:
    // Load error definitions from config directory
    // Each sensor type has its own file: <sensor>.errors
    // Default path: /etc/ws/sensor-errors/
    static void loadErrorDefinitions(const std::string& configDir = "/etc/ws/sensor-errors");
    
    // Helper to detect if a reading contains an error
    static bool isErrorReading(const std::map<std::string, std::string>& reading);
    
    // Get a description of the error, or empty string if no error
    static std::string getErrorDescription(const std::map<std::string, std::string>& reading);
    
private:
    static std::vector<ErrorDefinition> errorDefinitions;
    static bool definitionsLoaded;
    
    // Ensure definitions are loaded (lazy initialization)
    static void ensureLoaded();
};

#endif // ERROR_DETECTOR_H
