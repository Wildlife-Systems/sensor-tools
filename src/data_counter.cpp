#include "data_counter.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <map>
#include <ctime>

#include "csv_parser.h"
#include "json_parser.h"
#include "file_utils.h"
#include "file_collector.h"
#include "data_reader.h"

// Thread-safe helper to get tm struct from timestamp
static bool getTimeInfo(long long timestamp, struct tm& tm_info) {
    if (timestamp <= 0) return false;
    time_t t = static_cast<time_t>(timestamp);
#ifdef _WIN32
    if (gmtime_s(&tm_info, &t) != 0) return false;
#else
    if (gmtime_r(&t, &tm_info) == nullptr) return false;
#endif
    return true;
}

// Helper to extract YYYY-MM from a timestamp
static std::string timestampToMonth(long long timestamp) {
    struct tm tm_info;
    if (!getTimeInfo(timestamp, tm_info)) return "(no-date)";
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d", tm_info.tm_year + 1900, tm_info.tm_mon + 1);
    return std::string(buf);
}

// Helper to extract YYYY-MM-DD from a timestamp
static std::string timestampToDay(long long timestamp) {
    struct tm tm_info;
    if (!getTimeInfo(timestamp, tm_info)) return "(no-date)";
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday);
    return std::string(buf);
}

// Helper to extract YYYY from a timestamp
static std::string timestampToYear(long long timestamp) {
    struct tm tm_info;
    if (!getTimeInfo(timestamp, tm_info)) return "(no-date)";
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d", tm_info.tm_year + 1900);
    return std::string(buf);
}

// Helper to extract YYYY-Www from a timestamp (ISO week number)
static std::string timestampToWeek(long long timestamp) {
    struct tm tm_info;
    if (!getTimeInfo(timestamp, tm_info)) return "(no-date)";
    char buf[32];
    // Calculate ISO week number
    // ISO week: week 1 is the week containing the first Thursday of the year
    int year = tm_info.tm_year + 1900;
    int yday = tm_info.tm_yday;  // 0-365
    int wday = tm_info.tm_wday;  // 0=Sunday, 1=Monday, ..., 6=Saturday
    
    // Convert Sunday=0 to Monday=0 system (ISO uses Monday as first day)
    int isoWday = (wday == 0) ? 6 : wday - 1;  // 0=Monday, 6=Sunday
    
    // Find the Thursday of this week
    int thursdayYday = yday - isoWday + 3;  // Thursday is day 3 (0-indexed from Monday)
    
    // Handle year boundaries
    if (thursdayYday < 0) {
        // Thursday is in previous year
        year--;
        // Approximate: previous year has 365 or 366 days
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
    
    snprintf(buf, sizeof(buf), "%04d-W%02d", year, weekNum);
    return std::string(buf);
}

// ===== Private methods =====

long long DataCounter::countFromFile(const std::string& filename) {
    if (verbosity >= 1) {
        std::cerr << "Counting: " << filename << std::endl;
    }

    long long count = 0;
    std::unordered_map<std::string, long long> localValueCounts;  // Local counts for thread safety
    DataReader reader = createDataReader();
    
    reader.processFile(filename, [&](const Reading& reading, int /*lineNum*/, const std::string& /*source*/) {
        // Filtering already done by DataReader
        count++;
        if (!byColumn.empty()) {
            auto it = reading.find(byColumn);
            std::string value = (it != reading.end()) ? it->second : "(missing)";
            localValueCounts[value]++;
        }
        if (byMonth || byDay || byYear || byWeek) {
            long long ts = DateUtils::getTimestamp(reading);
            std::string period;
            if (byDay) {
                period = timestampToDay(ts);
            } else if (byWeek) {
                period = timestampToWeek(ts);
            } else if (byMonth) {
                period = timestampToMonth(ts);
            } else if (byYear) {
                period = timestampToYear(ts);
            }
            localValueCounts[period]++;
        }
    });

    // Merge local counts into shared valueCounts with mutex
    if ((!byColumn.empty() || byMonth || byDay || byYear || byWeek) && !localValueCounts.empty()) {
        std::lock_guard<std::mutex> lock(valueCountsMutex);
        for (const auto& pair : localValueCounts) {
            valueCounts[pair.first] += pair.second;
        }
    }

    return count;
}

long long DataCounter::countFromStdin() {
    if (verbosity >= 1) {
        std::cerr << "Reading from stdin..." << std::endl;
    }

    long long count = 0;
    std::unordered_map<std::string, long long> localValueCounts;  // Local counts for thread safety
    
    DataReader reader = createDataReader();
    
    reader.processStdin([&](const Reading& reading, int /*lineNum*/, const std::string& /*source*/) {
        // Filtering already done by DataReader
        count++;
        if (!byColumn.empty()) {
            auto it = reading.find(byColumn);
            std::string value = (it != reading.end()) ? it->second : "(missing)";
            localValueCounts[value]++;
        }
        if (byMonth || byDay || byYear || byWeek) {
            long long ts = DateUtils::getTimestamp(reading);
            std::string period;
            if (byDay) {
                period = timestampToDay(ts);
            } else if (byWeek) {
                period = timestampToWeek(ts);
            } else if (byMonth) {
                period = timestampToMonth(ts);
            } else if (byYear) {
                period = timestampToYear(ts);
            }
            localValueCounts[period]++;
        }
    });

    // Merge local counts into shared valueCounts with mutex
    if ((!byColumn.empty() || byMonth || byDay || byYear || byWeek) && !localValueCounts.empty()) {
        std::lock_guard<std::mutex> lock(valueCountsMutex);
        for (const auto& pair : localValueCounts) {
            valueCounts[pair.first] += pair.second;
        }
    }

    return count;
}

void DataCounter::countFromStdinFollow() {
    long long count = 0;
    
    // Print initial count
    std::cout << count << std::endl;
    
    // Use DataReader for parsing and filtering - only filtered readings reach callback
    auto reader = createDataReader();
    reader.processStdinFollow([&](const Reading& /*reading*/, int /*lineNum*/, const std::string& /*source*/) {
        count++;
        std::cout << count << std::endl;
    });
}

void DataCounter::countFromFileFollow(const std::string& filename) {
    long long count = 0;
    
    // Use DataReader for parsing and filtering - only filtered readings reach callback
    auto reader = createDataReader();
    reader.processFileFollow(filename, [&](const Reading& /*reading*/, int /*lineNum*/, const std::string& /*source*/) {
        count++;
        std::cout << count << std::endl;
    });
}

// ===== Constructor =====

DataCounter::DataCounter(int argc, char* argv[]) : followMode(false), byColumn(""), byMonth(false), byDay(false), byYear(false), byWeek(false), outputFormat("human"), outputFile("") {
    // Check for help flag first
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printCountUsage(argv[0]);
            exit(0);
        }
    }

    // Parse count-specific arguments and build filtered argv for CommonArgParser
    std::vector<char*> filteredArgv;
    for (int i = 0; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--follow" || arg == "-f") {
            followMode = true;
            continue;
        } else if (arg == "--by-month") {
            byMonth = true;
            continue;
        } else if (arg == "--by-day") {
            byDay = true;
            continue;
        } else if (arg == "--by-year") {
            byYear = true;
            continue;
        } else if (arg == "--by-week") {
            byWeek = true;
            continue;
        } else if ((arg == "--by-column" || arg == "-b") && i + 1 < argc) {
            byColumn = argv[i + 1];
            i++; // Skip the value
            continue;
        } else if ((arg == "--output-format" || arg == "-of") && i + 1 < argc) {
            outputFormat = argv[i + 1];
            if (outputFormat != "human" && outputFormat != "csv" && outputFormat != "json") {
                std::cerr << "Error: --output-format must be 'human', 'csv', or 'json'" << std::endl;
                exit(1);
            }
            i++; // Skip the value
            continue;
        } else if ((arg == "--output" || arg == "-o") && i + 1 < argc) {
            outputFile = argv[i + 1];
            i++; // Skip the value
            continue;
        }
        filteredArgv.push_back(argv[i]);
    }

    // Parse common flags and collect files using filtered argv
    CommonArgParser parser;
    if (!parser.parse(static_cast<int>(filteredArgv.size()), filteredArgv.data())) {
        exit(1);
    }

    copyFromParser(parser);

    // Check for unknown options using filtered argv
    std::string unknownOpt = CommonArgParser::checkUnknownOptions(
        static_cast<int>(filteredArgv.size()), filteredArgv.data());
    if (!unknownOpt.empty()) {
        std::cerr << "Error: Unknown option '" << unknownOpt << "'" << std::endl;
        printCountUsage(argv[0]);
        exit(1);
    }
}

// ===== Main count method =====

void DataCounter::count() {
    if (!hasInputFiles && followMode) {
        // Follow mode with stdin
        countFromStdinFollow();
        return;
    }
    
    if (followMode && inputFiles.size() == 1) {
        // Follow mode with a single file
        countFromFileFollow(inputFiles[0]);
        return;
    }
    
    if (followMode && inputFiles.size() > 1) {
        std::cerr << "Warning: --follow only supports a single file, using first file only" << std::endl;
        countFromFileFollow(inputFiles[0]);
        return;
    }
    
    long long totalCount = 0;

    if (!hasInputFiles) {
        totalCount = countFromStdin();
    } else {
        // Use parallel processing for multiple files
        totalCount = processFilesParallel<long long>(
            inputFiles,
            [this](const std::string& file) { return countFromFile(file); },
            [](long long& acc, long long val) { acc += val; },
            0LL,
            4  // numThreads
        );
    }

    // Set up output stream (file or stdout)
    std::ofstream fileStream;
    std::ostream* out = &std::cout;
    if (!outputFile.empty()) {
        fileStream.open(outputFile);
        if (!fileStream) {
            std::cerr << "Error: Cannot open output file '" << outputFile << "'" << std::endl;
            return;
        }
        out = &fileStream;
    }

    // Check if any time-based grouping is active
    bool timeGrouping = byMonth || byDay || byYear || byWeek;

    if (!byColumn.empty() || timeGrouping) {
        // Convert to vector and sort
        std::vector<std::pair<std::string, long long>> results(valueCounts.begin(), valueCounts.end());
        
        if (timeGrouping) {
            // Sort by time period ascending (YYYY-MM, YYYY-MM-DD, YYYY-Www, YYYY all sort correctly as strings)
            std::sort(results.begin(), results.end(), 
                [](const std::pair<std::string, long long>& a, const std::pair<std::string, long long>& b) {
                    return a.first < b.first;
                });
        } else {
            // Sort by count descending
            std::sort(results.begin(), results.end(), 
                [](const std::pair<std::string, long long>& a, const std::pair<std::string, long long>& b) {
                    return a.second > b.second;
                });
        }
        
        std::string columnLabel;
        if (byDay) columnLabel = "day";
        else if (byWeek) columnLabel = "week";
        else if (byMonth) columnLabel = "month";
        else if (byYear) columnLabel = "year";
        else columnLabel = byColumn;
        
        if (outputFormat == "json") {
            *out << "[";
            bool first = true;
            for (const auto& pair : results) {
                if (!first) *out << ",";
                first = false;
                *out << "{\"" << columnLabel << "\":\"" << pair.first 
                     << "\",\"count\":" << pair.second << "}";
            }
            *out << "]\n";
        } else if (outputFormat == "csv") {
            *out << columnLabel << ",count\n";
            for (const auto& pair : results) {
                *out << pair.first << "," << pair.second << "\n";
            }
        } else {
            // Human-readable format (default) - simple two-column output for time groupings
            if (timeGrouping) {
                for (const auto& pair : results) {
                    *out << pair.first << "\t" << pair.second << "\n";
                }
            } else {
                // Find max value width for alignment
                size_t maxValueWidth = columnLabel.length();
                for (const auto& pair : results) {
                    maxValueWidth = std::max(maxValueWidth, pair.first.length());
                }
                
                *out << "Counts by " << columnLabel << ":\n\n";
                *out << std::left;
                out->width(maxValueWidth + 2);
                *out << columnLabel;
                *out << "Count\n";
                *out << std::string(maxValueWidth + 2 + 10, '-') << "\n";
                
                for (const auto& pair : results) {
                    out->width(maxValueWidth + 2);
                    *out << pair.first;
                    *out << pair.second << "\n";
                }
                
                *out << "\nTotal: " << totalCount << " reading(s)\n";
            }
        }
    } else {
        *out << totalCount << std::endl;
    }

    if (!outputFile.empty()) {
        fileStream.close();
        std::cerr << "Output written to: " << outputFile << std::endl;
    }
}

// ===== Usage printing =====

void DataCounter::printCountUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " count [options] [files/directories...]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Count sensor data readings that match the specified filters." << std::endl;
    std::cerr << "Accepts the same filtering options as 'transform'." << std::endl;
    std::cerr << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  -o, --output <file>       Write output to file instead of stdout" << std::endl;
    std::cerr << "  -if, --input-format <fmt> Input format for stdin: json or csv (default: json)" << std::endl;
    std::cerr << "  -of, --output-format <fmt> Output format: human (default), csv, or json" << std::endl;
    std::cerr << "  -f, --follow              Follow mode: continuously monitor file/stdin for new data" << std::endl;
    std::cerr << "  -b, --by-column <col>     Show counts per value in the specified column" << std::endl;
    std::cerr << "  --by-day                  Show counts per day (YYYY-MM-DD format, ascending)" << std::endl;
    std::cerr << "  --by-week                 Show counts per week (YYYY-Www ISO week format, ascending)" << std::endl;
    std::cerr << "  --by-month                Show counts per month (YYYY-MM format, ascending)" << std::endl;
    std::cerr << "  --by-year                 Show counts per year (YYYY format, ascending)" << std::endl;
    std::cerr << "  -r, --recursive           Recursively process subdirectories" << std::endl;
    std::cerr << "  -v                        Verbose output (show progress)" << std::endl;
    std::cerr << "  -V                        Very verbose output (show detailed progress)" << std::endl;
    std::cerr << "  -e, --extension <ext>     Filter files by extension (e.g., .out or out)" << std::endl;
    std::cerr << "  -d, --depth <n>           Maximum recursion depth (0 = current dir only)" << std::endl;
    std::cerr << "  --not-empty <column>      Skip rows where column is empty (can be used multiple times)" << std::endl;
    std::cerr << "  --not-null <column>       Skip rows where column is 'null' (can be used multiple times)" << std::endl;
    std::cerr << "  --only-value <col:val>    Only include rows where column has specific value" << std::endl;
    std::cerr << "  --exclude-value <col:val> Exclude rows where column has specific value" << std::endl;
    std::cerr << "  --allowed-values <column> <values|file> Only include rows where column is in allowed values" << std::endl;
    std::cerr << "  --remove-errors           Remove error readings (DS18B20 value=85 or -127)" << std::endl;
    std::cerr << "  --remove-empty-json       Remove empty JSON input lines (e.g., [{}], [])" << std::endl;
    std::cerr << "  --clean                   Shorthand for --remove-empty-json --not-empty value --remove-errors --not-null value --not-null sensor_id" << std::endl;
    std::cerr << "  --min-date <date>         Filter readings after this date" << std::endl;
    std::cerr << "  --max-date <date>         Filter readings before this date" << std::endl;
    std::cerr << "  --tail <n>                Only read the last n lines from each file" << std::endl;
    std::cerr << "  --tail-column-value <col:val> <n>  Return last n rows where column=value" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Examples:" << std::endl;
    std::cerr << "  " << progName << " count sensor1.out" << std::endl;
    std::cerr << "  " << progName << " count < sensor1.out" << std::endl;
    std::cerr << "  " << progName << " count -r -e .out /path/to/logs" << std::endl;
    std::cerr << "  " << progName << " count --remove-errors sensor1.out" << std::endl;
    std::cerr << "  " << progName << " count --only-value type:temperature sensor1.out" << std::endl;
    std::cerr << "  " << progName << " count --allowed-values sensor_id allowed_sensors.txt sensor1.out" << std::endl;
    std::cerr << "  " << progName << " count --clean sensor.out  # exclude empty values" << std::endl;
    std::cerr << "  " << progName << " count --by-column sensor sensor1.out  # count per sensor" << std::endl;
    std::cerr << "  " << progName << " count --by-day -r -e out /path/to/logs    # count per day" << std::endl;
    std::cerr << "  " << progName << " count --by-week -r -e out /path/to/logs   # count per week" << std::endl;
    std::cerr << "  " << progName << " count --by-month -r -e out /path/to/logs  # count per month" << std::endl;
    std::cerr << "  " << progName << " count --by-year -r -e out /path/to/logs   # count per year" << std::endl;
    std::cerr << "  " << progName << " count --follow sensor.out" << std::endl;
    std::cerr << "  tail -f sensor.out | " << progName << " count --follow" << std::endl;
}
