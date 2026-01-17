#include "json_parser.h"
#include <cctype>

std::vector<std::map<std::string, std::string>> JsonParser::parseJsonLine(const std::string& line) {
    std::vector<std::map<std::string, std::string>> readings;
    
    // Find the main object or array
    size_t start = line.find('{');
    bool isArray = false;
    if (start == std::string::npos) {
        start = line.find('[');
        isArray = true;
    }
    
    if (start == std::string::npos) {
        return readings;
    }
    
    size_t pos = start;
    
    // If we start with an array, skip the '[' and look for objects
    if (isArray) {
        pos++;
        // Skip whitespace
        while (pos < line.length() && isspace(line[pos])) pos++;
    }
    
    // Parse all objects in the line (either one object or multiple in an array)
    while (pos < line.length()) {
        // Look for start of next object
        size_t objStart = line.find('{', pos);
        if (objStart == std::string::npos) break;
        
        pos = objStart + 1;
        std::map<std::string, std::string> current;
        
        // Parse key-value pairs for this object
        while (pos < line.length()) {
            // Skip whitespace and commas
            while (pos < line.length() && (isspace(line[pos]) || line[pos] == ',')) pos++;
            
            // Check for end of object
            if (pos < line.length() && line[pos] == '}') {
                pos++;
                break;
            }
            
            // Find key
            size_t keyStart = line.find('"', pos);
            if (keyStart == std::string::npos || keyStart >= line.length()) break;
            
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
                value = line.substr(valueStart + 1, valueEnd - valueStart - 1);
                pos = valueEnd + 1;
            } else if (line[valueStart] == '[') {
                // Array value
                size_t bracketEnd = line.find(']', valueStart);
                if (bracketEnd == std::string::npos) break;
                value = line.substr(valueStart + 1, bracketEnd - valueStart - 1);
                pos = bracketEnd + 1;
            } else {
                // Numeric or other value
                size_t valueEnd = line.find_first_of(",}]", valueStart);
                if (valueEnd == std::string::npos) valueEnd = line.length();
                value = line.substr(valueStart, valueEnd - valueStart);
                // Trim whitespace
                size_t end = value.find_last_not_of(" \t\n\r");
                if (end != std::string::npos) value = value.substr(0, end + 1);
                pos = valueEnd;
            }
            
            current[key] = value;
        }
        
        if (!current.empty()) {
            readings.push_back(current);
        }
        
        // Skip whitespace and commas after the object
        while (pos < line.length() && (isspace(line[pos]) || line[pos] == ',')) pos++;
        
        // Check if we've reached the end of the array
        if (pos < line.length() && line[pos] == ']') {
            break;
        }
    }
    
    return readings;
}

std::vector<std::map<std::string, std::string>> JsonParser::parseJsonArray(const std::string& jsonContent) {
    // For JSON array files like [ {...}, {...}, ... ]
    // Simply call parseJsonLine on the entire content
    // The existing parseJsonLine already handles arrays
    return parseJsonLine(jsonContent);
}
