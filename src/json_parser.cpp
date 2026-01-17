#include "json_parser.h"
#include <cctype>

std::vector<std::map<std::string, std::string>> JsonParser::parseJsonLine(const std::string& line) {
    std::vector<std::map<std::string, std::string>> readings;
    
    // Find the main object or array
    size_t start = line.find('{');
    if (start == std::string::npos) {
        start = line.find('[');
    }
    
    if (start == std::string::npos) {
        return readings;
    }
    
    // Simple JSON parsing - looks for key-value pairs
    std::map<std::string, std::string> current;
    size_t pos = start;
    
    while (pos < line.length()) {
        // Find key
        size_t keyStart = line.find('"', pos);
        if (keyStart == std::string::npos) break;
        
        size_t keyEnd = line.find('"', keyStart + 1);
        if (keyEnd == std::string::npos) break;
        
        std::string key = line.substr(keyStart + 1, keyEnd - keyStart - 1);
        
        // Find colon
        size_t colon = line.find(':', keyEnd);
        if (colon == std::string::npos) break;
        
        // Find value
        size_t valueStart = colon + 1;
        while (valueStart < line.length() && isspace(line[valueStart])) valueStart++;
        
        std::string value;
        if (line[valueStart] == '"') {
            // String value
            size_t valueEnd = line.find('"', valueStart + 1);
            if (valueEnd == std::string::npos) break;
            // Reserve space to reduce allocations
            value.reserve(valueEnd - valueStart - 1);
            value = line.substr(valueStart + 1, valueEnd - valueStart - 1);
            pos = valueEnd + 1;
        } else if (line[valueStart] == '[') {
            // Array - this might contain sensor readings
            size_t bracketEnd = line.find(']', valueStart);
            if (bracketEnd == std::string::npos) break;
            
            // If we have accumulated data and hit an array, might be multiple readings
            if (!current.empty() && key == "readings") {
                // Parse array of readings
                value.reserve(bracketEnd - valueStart - 1);
                value = line.substr(valueStart + 1, bracketEnd - valueStart - 1);
            }
            pos = bracketEnd + 1;
        } else {
            // Numeric or other value
            size_t valueEnd = line.find_first_of(",}]", valueStart);
            if (valueEnd == std::string::npos) valueEnd = line.length();
            value = line.substr(valueStart, valueEnd - valueStart);
            // Trim whitespace - optimize by finding actual end first
            size_t end = value.find_last_not_of(" \t\n\r");
            if (end != std::string::npos) value = value.substr(0, end + 1);
            pos = valueEnd;
        }
        
        current[key] = value;
        
        // Check if we hit end of object
        if (pos < line.length() && line[pos] == '}') {
            if (!current.empty()) {
                readings.push_back(current);
                current.clear();
            }
            pos++;
        }
    }
    
    if (!current.empty()) {
        readings.push_back(current);
    }
    
    // If no readings found, return empty
    if (readings.empty()) {
        readings.push_back(current);
    }
    
    return readings;
}
