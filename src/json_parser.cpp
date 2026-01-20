#include "json_parser.h"
#include <cctype>
#include <utility>

ReadingList JsonParser::parseJsonLine(const std::string& line) {
    ReadingList readings;
    readings.reserve(4);  // Most lines have 1-4 readings
    
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
        Reading current;
        current.reserve(8);  // Most readings have ~5-8 fields
        
        // Parse key-value pairs for this object
        while (pos < line.length()) {
            // Skip whitespace and commas
            while (pos < line.length() && (isspace(line[pos]) || line[pos] == ',')) pos++;
            
            // Check for end of object
            if (pos < line.length() && line[pos] == '}') {
                pos++;
                break;
            }
            
            // Find key (handle escaped quotes)
            size_t keyStart = line.find('"', pos);
            if (keyStart == std::string::npos || keyStart >= line.length()) break;
            
            size_t keyEnd = keyStart + 1;
            while (keyEnd < line.length()) {
                if (line[keyEnd] == '\\' && keyEnd + 1 < line.length()) {
                    keyEnd += 2;  // Skip escaped character
                } else if (line[keyEnd] == '"') {
                    break;
                } else {
                    keyEnd++;
                }
            }
            if (keyEnd >= line.length()) break;
            
            std::string key = line.substr(keyStart + 1, keyEnd - keyStart - 1);
            
            // Find colon
            size_t colon = line.find(':', keyEnd);
            if (colon == std::string::npos) break;
            
            // Find value
            size_t valueStart = colon + 1;
            while (valueStart < line.length() && isspace(line[valueStart])) valueStart++;
            
            std::string value;
            if (line[valueStart] == '"') {
                // String value - handle escaped quotes
                size_t i = valueStart + 1;
                while (i < line.length()) {
                    if (line[i] == '\\' && i + 1 < line.length()) {
                        i += 2;  // Skip escaped character
                    } else if (line[i] == '"') {
                        break;
                    } else {
                        i++;
                    }
                }
                if (i >= line.length()) break;
                value = line.substr(valueStart + 1, i - valueStart - 1);
                pos = i + 1;
            } else if (line[valueStart] == '[') {
                // Array value - find matching ], respecting nesting and strings
                int depth = 1;
                bool inString = false;
                size_t i = valueStart + 1;
                while (i < line.length() && depth > 0) {
                    if (inString) {
                        if (line[i] == '\\' && i + 1 < line.length()) {
                            i++;  // Skip escaped char
                        } else if (line[i] == '"') {
                            inString = false;
                        }
                    } else {
                        if (line[i] == '"') inString = true;
                        else if (line[i] == '[') depth++;
                        else if (line[i] == ']') depth--;
                    }
                    i++;
                }
                value = line.substr(valueStart + 1, i - valueStart - 2);
                pos = i;
            } else if (line[valueStart] == '{') {
                // Nested object value - find matching }, respecting nesting and strings
                int depth = 1;
                bool inString = false;
                size_t i = valueStart + 1;
                while (i < line.length() && depth > 0) {
                    if (inString) {
                        if (line[i] == '\\' && i + 1 < line.length()) {
                            i++;  // Skip escaped char
                        } else if (line[i] == '"') {
                            inString = false;
                        }
                    } else {
                        if (line[i] == '"') inString = true;
                        else if (line[i] == '{') depth++;
                        else if (line[i] == '}') depth--;
                    }
                    i++;
                }
                value = line.substr(valueStart, i - valueStart);  // Include braces
                pos = i;
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
            
            current.emplace(std::move(key), std::move(value));
        }
        
        if (!current.empty()) {
            readings.push_back(std::move(current));
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
