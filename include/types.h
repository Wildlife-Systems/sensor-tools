#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H

#include <string>
#include <unordered_map>
#include <vector>

// Use unordered_map for O(1) lookups instead of O(log n) with std::map
using Reading = std::unordered_map<std::string, std::string>;
using ReadingList = std::vector<Reading>;

#endif // SENSOR_TYPES_H
