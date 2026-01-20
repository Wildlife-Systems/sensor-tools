#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H

#include <string>
#include <unordered_map>
#include <vector>

// Use unordered_map for O(1) lookups instead of O(log n) with std::map
using Reading = std::unordered_map<std::string, std::string>;
using ReadingList = std::vector<Reading>;

// Column-oriented storage for memory efficiency with large datasets
// Each column is a vector of values, indexed by column name
using ColumnData = std::unordered_map<std::string, std::vector<std::string>>;

#endif // SENSOR_TYPES_H
