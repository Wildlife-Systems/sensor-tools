#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <string>
#include <map>
#include <vector>

class JsonParser {
public:
    // Parse a line of JSON - handles single objects, arrays, or line-delimited objects
    static std::vector<std::map<std::string, std::string>> parseJsonLine(const std::string& line);
};

#endif // JSON_PARSER_H
