#ifndef DATE_UTILS_H
#define DATE_UTILS_H

#include <string>
#include <ctime>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
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
    
    // The zone that times giving none of their own are read in, as an IANA name
    // such as Europe/London (set by --timezone). Empty means UTC.
    inline std::string& inputTimeZone() {
        static std::string zone;
        return zone;
    }

    // Days from 1970-01-01 to a date in the Gregorian calendar
    inline long long daysFromCivil(long long year, long long month, long long day) {
        year -= month <= 2 ? 1 : 0;
        const long long era = (year >= 0 ? year : year - 399) / 400;
        const long long yearOfEra = year - era * 400;
        const long long dayOfYear = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
        const long long dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
        return era * 146097 + dayOfEra - 719468;
    }

    // Unix time of a clock time in a zone offsetSeconds ahead of UTC
    inline long long wallClockToUnix(int year, int month, int day, int hour, int min, int sec,
                                     long long offsetSeconds) {
        return daysFromCivil(year, month, day) * 86400 + hour * 3600LL + min * 60LL + sec - offsetSeconds;
    }

    // Unix time of a clock time that gives no zone: in the --timezone zone if one
    // was set, otherwise in UTC
    inline long long zonelessToUnix(int year, int month, int day, int hour, int min, int sec) {
        if (inputTimeZone().empty()) {
            return wallClockToUnix(year, month, day, hour, min, sec, 0);
        }
        struct tm timeinfo = {};
        timeinfo.tm_year = year - 1900;
        timeinfo.tm_mon = month - 1;
        timeinfo.tm_mday = day;
        timeinfo.tm_hour = hour;
        timeinfo.tm_min = min;
        timeinfo.tm_sec = sec;
        timeinfo.tm_isdst = -1;  // the zone's rules decide whether it is summer time
        return static_cast<long long>(mktime(&timeinfo));
    }

    // Read times that give no zone in this IANA zone from now on; an empty name
    // goes back to UTC. This sets TZ for the whole process, so call it before any
    // threads start. Returns false, changing nothing, if the system's zoneinfo
    // does not have the zone.
    inline bool setInputTimeZone(const std::string& zone) {
#ifdef _WIN32
        if (!zone.empty()) return false;  // no zoneinfo to look the name up in
#else
        if (!zone.empty()) {
            if (zone.size() > 64 || zone[0] == '/' || zone.find("..") != std::string::npos) return false;
            for (char c : zone) {
                if (!std::isalnum(static_cast<unsigned char>(c)) && c != '/' && c != '_' && c != '-' && c != '+') {
                    return false;
                }
            }
            const char* tzdir = std::getenv("TZDIR");
            std::ifstream zoneFile(std::string(tzdir ? tzdir : "/usr/share/zoneinfo") + "/" + zone,
                                   std::ios::binary);
            char magic[4] = {};
            if (!zoneFile.read(magic, 4) || std::string(magic, 4) != "TZif") return false;
        }

        // Keep TZ as it was, to put it back when the zone is cleared
        static bool saved = false;
        static bool hadTz = false;
        static std::string originalTz;
        if (!saved) {
            const char* tz = std::getenv("TZ");
            hadTz = tz != nullptr;
            if (hadTz) originalTz = tz;
            saved = true;
        }
        if (!zone.empty()) {
            setenv("TZ", zone.c_str(), 1);
        } else if (hadTz) {
            setenv("TZ", originalTz.c_str(), 1);
        } else {
            unsetenv("TZ");
        }
        tzset();
#endif
        inputTimeZone() = zone;
        return true;
    }

    // Offset from UTC, in seconds, of a zone written after a time: Z, UTC, GMT,
    // +HH, +HHMM or +HH:MM (or the same with -), optionally after a space.
    // Returns false if the text is none of these.
    inline bool parseZoneOffset(const std::string& text, long long& offsetSeconds) {
        std::string zone = (!text.empty() && text[0] == ' ') ? text.substr(1) : text;
        std::string upper;
        for (char c : zone) upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (upper == "Z" || upper == "UTC" || upper == "GMT") {
            offsetSeconds = 0;
            return true;
        }
        if (zone.size() < 3 || (zone[0] != '+' && zone[0] != '-')) return false;
        std::string digits;
        for (size_t i = 1; i < zone.size(); ++i) {
            if (zone[i] == ':' && i == 3) continue;
            if (!std::isdigit(static_cast<unsigned char>(zone[i]))) return false;
            digits += zone[i];
        }
        if (digits.size() != 2 && digits.size() != 4) return false;
        const int hours = std::stoi(digits.substr(0, 2));
        const int minutes = digits.size() == 4 ? std::stoi(digits.substr(2, 2)) : 0;
        if (hours > 14 || minutes > 59) return false;
        offsetSeconds = (hours * 3600LL + minutes * 60LL) * (zone[0] == '-' ? -1 : 1);
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
                return zonelessToUnix(year, month, day, defaultHour, defaultMin, defaultSec);
            }
        }
        
        // Try ISO-like formats:
        // - YYYY-MM-DD
        // - YYYY-MM-DDTHH:MM[:SS][.sss][Z|+HH:MM]
        // - YYYY-MM-DD HH:MM[:SS][.sss][Z|+HH:MM]
        if (dateStr.length() >= 10 && dateStr[4] == '-' && dateStr[7] == '-') {
            int year, month, day;
            if (sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &day) == 3) {
                int hour = defaultHour;
                int min = defaultMin;
                int sec = defaultSec;

                // Parse optional time component after position 10, then any zone after it
                bool hasZone = false;
                long long offsetSeconds = 0;
                if (dateStr.length() > 10) {
                    char sep = dateStr[10];
                    if (sep == 'T' || sep == 't' || sep == ' ') {
                        int parsedHour = 0;
                        int parsedMin = 0;
                        int used = 0;
                        if (sscanf(dateStr.c_str() + 11, "%d:%d%n", &parsedHour, &parsedMin, &used) == 2) {
                            hour = parsedHour;
                            min = parsedMin;
                            sec = 0;
                            size_t pos = 11 + static_cast<size_t>(used);
                            int parsedSec = 0;
                            if (pos < dateStr.length() && dateStr[pos] == ':' &&
                                sscanf(dateStr.c_str() + pos + 1, "%d%n", &parsedSec, &used) == 1) {
                                sec = parsedSec;
                                pos += 1 + static_cast<size_t>(used);
                            }
                            // Fractions of a second are dropped
                            if (pos < dateStr.length() && (dateStr[pos] == '.' || dateStr[pos] == ',')) {
                                do {
                                    ++pos;
                                } while (pos < dateStr.length() && std::isdigit(static_cast<unsigned char>(dateStr[pos])));
                            }
                            hasZone = parseZoneOffset(dateStr.substr(pos), offsetSeconds);
                        }
                    }
                }
                
                if (!isValidDateTime(year, month, day, hour, min, sec)) return 0;
                
                if (hasZone) {
                    return wallClockToUnix(year, month, day, hour, min, sec, offsetSeconds);
                }
                return zonelessToUnix(year, month, day, hour, min, sec);
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
