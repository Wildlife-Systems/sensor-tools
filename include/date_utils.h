#ifndef DATE_UTILS_H
#define DATE_UTILS_H

#include <string>
#include <map>
#include <ctime>
#include <cctype>

// Date/Time utility functions
namespace DateUtils {
    // Validate date/time component ranges
    inline bool isValidDateTime(int year, int month, int day, int hour = 0, int min = 0, int sec = 0) {
        if (year < 1970 || year > 2100) return false;
        if (month < 1 || month > 12) return false;
        if (day < 1 || day > 31) return false;
        if (hour < 0 || hour > 23) return false;
        if (min < 0 || min > 59) return false;
        if (sec < 0 || sec > 59) return false;
        
        // More precise day validation based on month
        int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        // Leap year check
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            daysInMonth[1] = 29;
        }
        if (day > daysInMonth[month - 1]) return false;
        
        return true;
    }

    // Parse date string to Unix timestamp
    // Supports: Unix timestamp, ISO 8601 (YYYY-MM-DD or YYYY-MM-DDTHH:MM:SS), DD/MM/YYYY
    inline long long parseDate(const std::string& dateStr) {
        if (dateStr.empty()) return 0;
        
        // Try DD/MM/YYYY format first
        if (dateStr.find('/') != std::string::npos) {
            int day, month, year;
            if (sscanf(dateStr.c_str(), "%d/%d/%d", &day, &month, &year) == 3) {
                if (!isValidDateTime(year, month, day)) return 0;
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
        // Check for pattern like XXXX-XX-XX (4 digits, dash, 2 digits, dash, 2 digits)
        if (dateStr.length() >= 10 && dateStr[4] == '-' && dateStr[7] == '-') {
            int year, month, day, hour = 0, min = 0, sec = 0;
            if (sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &day) >= 3) {
                // Check if there's a time component
                size_t tPos = dateStr.find('T');
                if (tPos != std::string::npos) {
                    sscanf(dateStr.c_str() + tPos + 1, "%d:%d:%d", &hour, &min, &sec);
                }
                
                if (!isValidDateTime(year, month, day, hour, min, sec)) return 0;
                
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
        
        // Try as Unix timestamp (all digits, optionally with leading minus)
        bool isTimestamp = true;
        size_t start = 0;
        if (!dateStr.empty() && dateStr[0] == '-') {
            start = 1;
        }
        for (size_t i = start; i < dateStr.length(); ++i) {
            if (!std::isdigit(dateStr[i])) {
                isTimestamp = false;
                break;
            }
        }
        if (isTimestamp && dateStr.length() > start) {
            try {
                return std::stoll(dateStr);
            } catch (...) {}
        }
        
        return 0;
    }
    
    // Parse date string to Unix timestamp, using end of day (23:59:59) for date-only inputs
    // Used for --max-date to include the entire specified day
    inline long long parseDateEndOfDay(const std::string& dateStr) {
        if (dateStr.empty()) return 0;
        
        // Try DD/MM/YYYY format first - use end of day
        if (dateStr.find('/') != std::string::npos) {
            int day, month, year;
            if (sscanf(dateStr.c_str(), "%d/%d/%d", &day, &month, &year) == 3) {
                if (!isValidDateTime(year, month, day)) return 0;
                struct tm timeinfo = {};
                timeinfo.tm_year = year - 1900;
                timeinfo.tm_mon = month - 1;
                timeinfo.tm_mday = day;
                timeinfo.tm_hour = 23;
                timeinfo.tm_min = 59;
                timeinfo.tm_sec = 59;
                return static_cast<long long>(mktime(&timeinfo));
            }
        }
        
        // Try ISO 8601 format (YYYY-MM-DD or YYYY-MM-DDTHH:MM:SS)
        if (dateStr.length() >= 10 && dateStr[4] == '-' && dateStr[7] == '-') {
            int year, month, day, hour = 23, min = 59, sec = 59;
            if (sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &day) >= 3) {
                // Check if there's a time component - use specified time
                size_t tPos = dateStr.find('T');
                if (tPos != std::string::npos) {
                    sscanf(dateStr.c_str() + tPos + 1, "%d:%d:%d", &hour, &min, &sec);
                }
                // else: no time specified, use end of day (23:59:59)
                
                if (!isValidDateTime(year, month, day, hour, min, sec)) return 0;
                
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
        
        // For Unix timestamps, use as-is
        bool isTimestamp = true;
        size_t start = 0;
        if (!dateStr.empty() && dateStr[0] == '-') {
            start = 1;
        }
        for (size_t i = start; i < dateStr.length(); ++i) {
            if (!std::isdigit(dateStr[i])) {
                isTimestamp = false;
                break;
            }
        }
        if (isTimestamp && dateStr.length() > start) {
            try {
                return std::stoll(dateStr);
            } catch (...) {}
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
