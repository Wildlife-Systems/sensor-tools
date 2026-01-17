#ifndef DATE_UTILS_H
#define DATE_UTILS_H

#include <string>
#include <map>
#include <ctime>
#include <cctype>

// Date/Time utility functions
namespace DateUtils {
    // Parse date string to Unix timestamp
    // Supports: Unix timestamp, ISO 8601 (YYYY-MM-DD or YYYY-MM-DDTHH:MM:SS), DD/MM/YYYY
    inline long long parseDate(const std::string& dateStr) {
        if (dateStr.empty()) return 0;
        
        // Try as Unix timestamp (all digits)
        bool isDigits = true;
        for (char c : dateStr) {
            if (!std::isdigit(c) && c != '-') {
                isDigits = false;
                break;
            }
        }
        if (isDigits && dateStr.find('/') == std::string::npos && dateStr.find('T') == std::string::npos) {
            try {
                return std::stoll(dateStr);
            } catch (...) {}
        }
        
        // Try DD/MM/YYYY format
        if (dateStr.find('/') != std::string::npos) {
            int day, month, year;
            if (sscanf(dateStr.c_str(), "%d/%d/%d", &day, &month, &year) == 3) {
                struct tm timeinfo = {};
                timeinfo.tm_year = year - 1900;
                timeinfo.tm_mon = month - 1;
                timeinfo.tm_mday = day;
                timeinfo.tm_hour = 0;
                timeinfo.tm_min = 0;
                timeinfo.tm_sec = 0;
                return static_cast<long long>(mktime(&timeinfo));
            }
        }
        
        // Try ISO 8601 format (YYYY-MM-DD or YYYY-MM-DDTHH:MM:SS)
        if (dateStr.find('-') != std::string::npos) {
            int year, month, day, hour = 0, min = 0, sec = 0;
            if (sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &day) >= 3) {
                // Check if there's a time component
                size_t tPos = dateStr.find('T');
                if (tPos != std::string::npos) {
                    sscanf(dateStr.c_str() + tPos + 1, "%d:%d:%d", &hour, &min, &sec);
                }
                
                struct tm timeinfo = {};
                timeinfo.tm_year = year - 1900;
                timeinfo.tm_mon = month - 1;
                timeinfo.tm_mday = day;
                timeinfo.tm_hour = hour;
                timeinfo.tm_min = min;
                timeinfo.tm_sec = sec;
                return static_cast<long long>(mktime(&timeinfo));
            }
        }
        
        return 0;
    }
    
    // Parse timestamp from reading (handles both string and numeric timestamps)
    inline long long getTimestamp(const std::map<std::string, std::string>& reading) {
        auto it = reading.find("timestamp");
        if (it == reading.end() || it->second.empty()) {
            return 0;
        }
        return parseDate(it->second);
    }
    
    // Check if timestamp is within date range
    inline bool isInDateRange(long long timestamp, long long minDate, long long maxDate) {
        if (timestamp == 0) return true;  // No timestamp, include by default
        if (minDate > 0 && timestamp < minDate) return false;
        if (maxDate > 0 && timestamp > maxDate) return false;
        return true;
    }
}

#endif // DATE_UTILS_H
