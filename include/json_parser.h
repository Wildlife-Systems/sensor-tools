#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <string>
#include <map>
#include <vector>

class JsonParser {
public:
    static std::vector<std::map<std::string, std::string>> parseJsonLine(const std::string& line);
    static std::vector<std::map<std::string, std::string>> parseJsonArray(const std::string& jsonContent);
};

#endif // JSON_PARSER_H
