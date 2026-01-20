#ifndef READING_FILTER_H
#define READING_FILTER_H

#include <string>
#include <set>
#include <map>
#include <cstring>
#include <iostream>

#include "types.h"
#include "date_utils.h"
#include "error_detector.h"

/**
 * ReadingFilter - Centralized filtering for sensor readings.
 * 
 * This class encapsulates ALL filtering logic so it can be applied
 * consistently across all commands via DataReader.
 * 
 * Flow: Input → Parse → ReadingFilter → Command-specific logic
 */
class ReadingFilter {
private:
    // Date filtering
    long long minDate;
    long long maxDate;
    
    // Error filtering
    bool removeErrors;
    
    // Empty/null filtering
    std::set<std::string> notEmptyColumns;
    std::set<std::string> notNullColumns;
    
    // Value-based filtering
    std::map<std::string, std::set<std::string>> onlyValueFilters;
    std::map<std::string, std::set<std::string>> excludeValueFilters;
    std::map<std::string, std::set<std::string>> allowedValues;
    
    // Invert mode (for list-rejects command)
    bool invertFilter;
    
    // Debug output
    int verbosity;

public:
    ReadingFilter()
        : minDate(0)
        , maxDate(0)
        , removeErrors(false)
        , invertFilter(false)
        , verbosity(0) {}
    
    // Setters for filter configuration
    void setDateRange(long long min, long long max) {
        minDate = min;
        maxDate = max;
    }
    
    void setRemoveErrors(bool remove) {
        removeErrors = remove;
    }
    
    void setVerbosity(int v) {
        verbosity = v;
    }
    
    void addNotEmptyColumn(const std::string& col) {
        notEmptyColumns.insert(col);
    }
    
    void addNotNullColumn(const std::string& col) {
        notNullColumns.insert(col);
    }
    
    void addOnlyValueFilter(const std::string& col, const std::string& value) {
        onlyValueFilters[col].insert(value);
    }
    
    void addExcludeValueFilter(const std::string& col, const std::string& value) {
        excludeValueFilters[col].insert(value);
    }
    
    void addAllowedValue(const std::string& col, const std::string& value) {
        allowedValues[col].insert(value);
    }
    
    // Bulk setters
    void setNotEmptyColumns(const std::set<std::string>& cols) {
        notEmptyColumns = cols;
    }
    
    void setNotNullColumns(const std::set<std::string>& cols) {
        notNullColumns = cols;
    }
    
    void setOnlyValueFilters(const std::map<std::string, std::set<std::string>>& filters) {
        onlyValueFilters = filters;
    }
    
    void setExcludeValueFilters(const std::map<std::string, std::set<std::string>>& filters) {
        excludeValueFilters = filters;
    }
    
    void setAllowedValues(const std::map<std::string, std::set<std::string>>& values) {
        allowedValues = values;
    }
    
    void setInvertFilter(bool invert) {
        invertFilter = invert;
    }
    
    /**
     * Check if a reading passes the date filter
     */
    bool passesDateFilter(const Reading& reading) const {
        if (minDate <= 0 && maxDate <= 0) return true;
        long long timestamp = DateUtils::getTimestamp(reading);
        return DateUtils::isInDateRange(timestamp, minDate, maxDate);
    }
    
    /**
     * Internal check - does reading pass all filter criteria?
     */
    bool passesAllFilters(const Reading& reading) const {
        // Check date range
        if (!passesDateFilter(reading)) {
            if (verbosity >= 2) {
                std::cerr << "  Skipping row: outside date range" << std::endl;
            }
            return false;
        }
        
        // Check if any required columns are empty
        for (const auto& reqCol : notEmptyColumns) {
            auto it = reading.find(reqCol);
            if (it == reading.end() || it->second.empty()) {
                if (verbosity >= 2) {
                    std::cerr << "  Skipping row: " 
                             << (it == reading.end() ? "missing column '" : "empty column '") 
                             << reqCol << "'" << std::endl;
                }
                return false;
            }
        }
        
        // Check if any required columns contain null values
        for (const auto& reqCol : notNullColumns) {
            auto it = reading.find(reqCol);
            if (it != reading.end()) {
                const std::string& val = it->second;
                bool hasNullChar = (memchr(val.data(), '\0', val.size()) != nullptr);
                if (val == "null" || hasNullChar) {
                    if (verbosity >= 2) {
                        std::cerr << "  Skipping row: null value in column '" << reqCol << "'" << std::endl;
                    }
                    return false;
                }
            }
        }
        
        // Check value filters (include only)
        for (const auto& filter : onlyValueFilters) {
            auto it = reading.find(filter.first);
            if (it == reading.end() || filter.second.count(it->second) == 0) {
                if (verbosity >= 2) {
                    if (it == reading.end()) {
                        std::cerr << "  Skipping row: missing column '" << filter.first << "'" << std::endl;
                    } else {
                        std::cerr << "  Skipping row: column '" << filter.first << "' has value '" 
                                 << it->second << "' (not in allowed values)" << std::endl;
                    }
                }
                return false;
            }
        }
        
        // Check value filters (exclude)
        for (const auto& filter : excludeValueFilters) {
            auto it = reading.find(filter.first);
            if (it != reading.end() && filter.second.count(it->second) > 0) {
                if (verbosity >= 2) {
                    std::cerr << "  Skipping row: column '" << filter.first << "' has excluded value '" 
                             << it->second << "'" << std::endl;
                }
                return false;
            }
        }
        
        // Check allowed values filters
        for (const auto& filter : allowedValues) {
            auto it = reading.find(filter.first);
            if (it == reading.end() || filter.second.count(it->second) == 0) {
                if (verbosity >= 2) {
                    if (it == reading.end()) {
                        std::cerr << "  Skipping row: missing column '" << filter.first << "'" << std::endl;
                    } else {
                        std::cerr << "  Skipping row: column '" << filter.first << "' value '" 
                                 << it->second << "' not in allowed values" << std::endl;
                    }
                }
                return false;
            }
        }
        
        // Check for error readings
        if (removeErrors && ErrorDetector::isErrorReading(reading)) {
            if (verbosity >= 2) {
                std::string errorDesc = ErrorDetector::getErrorDescription(reading);
                std::cerr << "  Skipping error reading: " << errorDesc << std::endl;
            }
            return false;
        }
        
        return true;
    }
    
    /**
     * Check if a reading should be included based on ALL active filters.
     * This is the single point where all filtering decisions are made.
     * If invertFilter is true, returns readings that FAIL the filters (for list-rejects).
     */
    bool shouldInclude(const Reading& reading) const {
        bool passes = passesAllFilters(reading);
        return invertFilter ? !passes : passes;
    }
};

#endif // READING_FILTER_H
