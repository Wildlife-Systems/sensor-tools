#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <string>
#include <vector>
#include "types.h"

class JsonParser {
public:
    // Parse a line of JSON - handles single objects, arrays, or line-delimited objects
    static ReadingList parseJsonLine(const std::string& line);
};
};

#endif // JSON_PARSER_H
