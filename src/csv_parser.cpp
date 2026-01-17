#include "csv_parser.h"

std::vector<std::string> CsvParser::parseCsvLine(std::istream& input, std::string& line, bool& needMoreLines) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;
    needMoreLines = false;
    
    while (true) {
        for (size_t i = 0; i < line.length(); ++i) {
            char c = line[i];
            
            if (inQuotes) {
                if (c == '"') {
                    // Check if it's an escaped quote
                    if (i + 1 < line.length() && line[i + 1] == '"') {
                        current += '"';
                        ++i;  // Skip next quote
                    } else {
                        inQuotes = false;
                    }
                } else {
                    current += c;
                }
            } else {
                if (c == '"') {
                    inQuotes = true;
                } else if (c == ',') {
                    fields.push_back(current);
                    current.clear();
                } else if (c != '\r') {  // Ignore CR in CRLF
                    current += c;
                }
            }
        }
        
        // If we're still in quotes, we need to read more lines
        if (inQuotes) {
            current += '\n';  // Add newline that was consumed by getline
            if (std::getline(input, line)) {
                needMoreLines = true;
                continue;  // Continue parsing with the next line
            } else {
                // End of file while in quotes - malformed CSV
                break;
            }
        } else {
            break;  // Done parsing this logical line
        }
    }
    
    fields.push_back(current);
    needMoreLines = false;
    return fields;
}

std::vector<std::string> CsvParser::parseCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;
    
    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];
        
        if (inQuotes) {
            if (c == '"') {
                // Check if it's an escaped quote
                if (i + 1 < line.length() && line[i + 1] == '"') {
                    current += '"';
                    ++i;  // Skip next quote
                } else {
                    inQuotes = false;
                }
            } else {
                current += c;
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                fields.push_back(current);
                current.clear();
            } else if (c != '\r') {  // Ignore CR in CRLF
                current += c;
            }
        }
    }
    
    fields.push_back(current);
    return fields;
}
