#ifndef DATE_UTILS_H
#define DATE_UTILS_H

#include <string>
#include <ctime>
#include <cctype>
#include "types.h"

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
    
    // Internal helper for parsing dates with configurable default time
    // defaultHour/Min/Sec used when no time component is specified
    inline long long parseDateInternal(const std::string& dateStr, int defaultHour, int defaultMin, int defaultSec) {
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
                timeinfo.tm_hour = defaultHour;
                timeinfo.tm_min = defaultMin;
                timeinfo.tm_sec = defaultSec;
                return static_cast<long long>(mktime(&timeinfo));
            }
        }
        
        // Try ISO 8601 format (YYYY-MM-DD or YYYY-MM-DDTHH:MM:SS)
        if (dateStr.length() >= 10 && dateStr[4] == '-' && dateStr[7] == '-') {
            int year, month, day, hour = defaultHour, min = defaultMin, sec = defaultSec;
            if (sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &day) >= 3) {
                // Check if there's a time component - use specified time
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

    // Parse date string to Unix timestamp (start of day for date-only inputs)
    inline long long parseDate(const std::string& dateStr) {
        return parseDateInternal(dateStr, 0, 0, 0);
    }
    
    // Parse date string to Unix timestamp, using end of day (23:59:59) for date-only inputs
    // Used for --max-date to include the entire specified day
    inline long long parseDateEndOfDay(const std::string& dateStr) {
        return parseDateInternal(dateStr, 23, 59, 59);
    }
    
    // Parse timestamp from reading (handles both string and numeric timestamps)
    inline long long getTimestamp(const Reading& reading) {
        auto it = reading.find("timestamp");
        if (it == reading.end() || it->second.empty()) {
            return 0;
        }
        return parseDate(it->second);
    }
    
    // Check if timestamp is within date range
    inline bool isInDateRange(long long timestamp, long long minDate, long long maxDate) {
        if (timestamp == 0) return false;  // No timestamp, exclude when date filter is active
        if (minDate > 0 && timestamp < minDate) return false;
        if (maxDate > 0 && timestamp > maxDate) return false;
        return true;
    }

    // Thread-safe helper to get tm struct from timestamp
    inline bool getTimeInfo(long long timestamp, struct tm& tm_info) {
        if (timestamp <= 0) return false;
        time_t t = static_cast<time_t>(timestamp);
#ifdef _WIN32
        if (gmtime_s(&tm_info, &t) != 0) return false;
#else
        if (gmtime_r(&t, &tm_info) == nullptr) return false;
#endif
        return true;
    }

    // Convert timestamp to YYYY-MM format
    inline std::string timestampToMonth(long long timestamp) {
        struct tm tm_info;
        if (!getTimeInfo(timestamp, tm_info)) return "(no-date)";
        char buf[32];
        snprintf(buf, sizeof(buf), "%04d-%02d", tm_info.tm_year + 1900, tm_info.tm_mon + 1);
        return std::string(buf);
    }

    // Convert timestamp to YYYY-MM-DD format
    inline std::string timestampToDay(long long timestamp) {
        struct tm tm_info;
        if (!getTimeInfo(timestamp, tm_info)) return "(no-date)";
        char buf[32];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday);
        return std::string(buf);
    }

    // Convert timestamp to YYYY format
    inline std::string timestampToYear(long long timestamp) {
        struct tm tm_info;
        if (!getTimeInfo(timestamp, tm_info)) return "(no-date)";
        char buf[32];
        snprintf(buf, sizeof(buf), "%04d", tm_info.tm_year + 1900);
        return std::string(buf);
    }

    // Convert timestamp to YYYY-Www format (ISO week number)
    // Manual calculation since %G and %V aren't portable
    inline std::string timestampToWeek(long long timestamp) {
        struct tm tm_info;
        if (!getTimeInfo(timestamp, tm_info)) return "(no-date)";
        
        int year = tm_info.tm_year + 1900;
        int yday = tm_info.tm_yday;  // 0-365
        int wday = tm_info.tm_wday;  // 0=Sunday, 6=Saturday
        
        // Convert Sunday=0 to Monday=0 system (ISO uses Monday as first day)
        int isoWday = (wday == 0) ? 6 : wday - 1;  // 0=Monday, 6=Sunday
        
        // Find the Thursday of this week
        int thursdayYday = yday - isoWday + 3;  // Thursday is day 3 (0-indexed from Monday)
        
        // Handle year boundaries
        if (thursdayYday < 0) {
            // Thursday is in previous year
            year--;
            bool prevLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
            thursdayYday += prevLeap ? 366 : 365;
        } else {
            bool thisLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
            int daysInYear = thisLeap ? 366 : 365;
            if (thursdayYday >= daysInYear) {
                // Thursday is in next year
                year++;
                thursdayYday -= daysInYear;
            }
        }
        
        // Week number is (thursdayYday / 7) + 1
        int weekNum = (thursdayYday / 7) + 1;
        
        char buf[32];
        snprintf(buf, sizeof(buf), "%04d-W%02d", year, weekNum);
        return std::string(buf);
    }
}

#endif // DATE_UTILS_H
