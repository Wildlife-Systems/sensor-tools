#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include <string>
#include <vector>
#include <iostream>

class CsvParser {
public:
    // Parse CSV line considering proper escaping and quoting
    // Note: This parser handles multi-line fields by reading additional lines when needed
    static std::vector<std::string> parseCsvLine(std::istream& input, std::string& line, bool& needMoreLines);
    
    // Simpler version for single-line parsing (backwards compatibility)
    static std::vector<std::string> parseCsvLine(const std::string& line);
};

#endif // CSV_PARSER_H
