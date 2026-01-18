#ifndef DATA_COUNTER_H
#define DATA_COUNTER_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cstdlib>

#include "date_utils.h"
#include "common_arg_parser.h"
#include "csv_parser.h"
#include "json_parser.h"
#include "error_detector.h"
#include "file_utils.h"

class DataCounter {
private:
    std::vector<std::string> inputFiles;
    bool hasInputFiles;
    bool recursive;
    std::string extensionFilter;
    int maxDepth;
    int verbosity;
    bool removeErrors;
    bool removeEmptyJson;
    std::string inputFormat;
    std::set<std::string> notEmptyColumns;
    std::map<std::string, std::set<std::string>> onlyValueFilters;
    std::map<std::string, std::set<std::string>> excludeValueFilters;
    long long minDate;
    long long maxDate;

    // Check if a reading passes date filter
    bool passesDateFilter(const std::map<std::string, std::string>& reading) const {
        if (minDate > 0 || maxDate > 0) {
            long long timestamp = DateUtils::getTimestamp(reading);
            return DateUtils::isInDateRange(timestamp, minDate, maxDate);
        }
        return true;
    }

    // Check if a reading should be included based on filters
    bool shouldIncludeReading(const std::map<std::string, std::string>& reading) {
        // Check date range
        if (!passesDateFilter(reading)) {
            return false;
        }

        // Check if any required columns are empty
        for (const auto& reqCol : notEmptyColumns) {
            auto it = reading.find(reqCol);
            if (it == reading.end() || it->second.empty()) {
                return false;
            }
        }

        // Check value filters (include)
        for (const auto& filter : onlyValueFilters) {
            auto it = reading.find(filter.first);
            if (it == reading.end() || filter.second.count(it->second) == 0) {
                return false;
            }
        }

        // Check value filters (exclude)
        for (const auto& filter : excludeValueFilters) {
            auto it = reading.find(filter.first);
            if (it != reading.end() && filter.second.count(it->second) > 0) {
                return false;
            }
        }

        // Check for error readings
        if (removeErrors && ErrorDetector::isErrorReading(reading)) {
            return false;
        }

        return true;
    }

    // Count readings from a single file
    long long countFromFile(const std::string& filename) {
        if (verbosity >= 1) {
            std::cerr << "Counting: " << filename << std::endl;
        }

        std::ifstream infile(filename);
        if (!infile) {
            std::cerr << "Warning: Cannot open file: " << filename << std::endl;
            return 0;
        }

        long long count = 0;
        std::string line;

        if (FileUtils::isCsvFile(filename)) {
            // CSV format - first line is header
            std::vector<std::string> csvHeaders;
            if (std::getline(infile, line) && !line.empty()) {
                bool needMore = false;
                csvHeaders = CsvParser::parseCsvLine(infile, line, needMore);
            }

            // Process data rows
            while (std::getline(infile, line)) {
                if (line.empty()) continue;

                bool needMore = false;
                auto fields = CsvParser::parseCsvLine(infile, line, needMore);
                if (fields.empty()) continue;

                // Create a map from CSV headers to values
                std::map<std::string, std::string> reading;
                for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                    reading[csvHeaders[i]] = fields[i];
                }

                if (shouldIncludeReading(reading)) {
                    count++;
                }
            }
        } else {
            // JSON format
            while (std::getline(infile, line)) {
                if (line.empty()) continue;

                auto readings = JsonParser::parseJsonLine(line);
                
                // Skip empty JSON arrays/objects if removeEmptyJson is set
                if (removeEmptyJson && readings.empty()) {
                    continue;
                }

                for (const auto& reading : readings) {
                    if (reading.empty()) continue;
                    if (shouldIncludeReading(reading)) {
                        count++;
                    }
                }
            }
        }

        infile.close();
        return count;
    }

    // Count readings from stdin
    long long countFromStdin() {
        if (verbosity >= 1) {
            std::cerr << "Reading from stdin..." << std::endl;
        }

        long long count = 0;
        std::string line;

        if (inputFormat == "csv") {
            // CSV format - first line is header
            std::vector<std::string> csvHeaders;
            if (std::getline(std::cin, line) && !line.empty()) {
                csvHeaders = CsvParser::parseCsvLine(line);
            }

            while (std::getline(std::cin, line)) {
                if (line.empty()) continue;

                auto fields = CsvParser::parseCsvLine(line);
                if (fields.empty()) continue;

                std::map<std::string, std::string> reading;
                for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                    reading[csvHeaders[i]] = fields[i];
                }

                if (shouldIncludeReading(reading)) {
                    count++;
                }
            }
        } else {
            // JSON format
            while (std::getline(std::cin, line)) {
                if (line.empty()) continue;

                auto readings = JsonParser::parseJsonLine(line);
                
                if (removeEmptyJson && readings.empty()) {
                    continue;
                }

                for (const auto& reading : readings) {
                    if (reading.empty()) continue;
                    if (shouldIncludeReading(reading)) {
                        count++;
                    }
                }
            }
        }

        return count;
    }

    void printCountUsage(const char* progName) {
        std::cerr << "Usage: " << progName << " count [options] [files/directories...]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Count sensor data readings that match the specified filters." << std::endl;
        std::cerr << "Accepts the same filtering options as 'transform'." << std::endl;
        std::cerr << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << "  -f, --format <fmt>        Input format for stdin: json or csv (default: json)" << std::endl;
        std::cerr << "  -r, --recursive           Recursively process subdirectories" << std::endl;
        std::cerr << "  -v                        Verbose output (show progress)" << std::endl;
        std::cerr << "  -V                        Very verbose output (show detailed progress)" << std::endl;
        std::cerr << "  -e, --extension <ext>     Filter files by extension (e.g., .out or out)" << std::endl;
        std::cerr << "  -d, --depth <n>           Maximum recursion depth (0 = current dir only)" << std::endl;
        std::cerr << "  --not-empty <column>      Skip rows where column is empty (can be used multiple times)" << std::endl;
        std::cerr << "  --only-value <col:val>    Only include rows where column has specific value" << std::endl;
        std::cerr << "  --exclude-value <col:val> Exclude rows where column has specific value" << std::endl;
        std::cerr << "  --remove-errors           Remove error readings (DS18B20 value=85 or -127)" << std::endl;
        std::cerr << "  --remove-empty-json       Remove empty JSON input lines (e.g., [{}], [])" << std::endl;
        std::cerr << "  --min-date <date>         Filter readings after this date" << std::endl;
        std::cerr << "  --max-date <date>         Filter readings before this date" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Examples:" << std::endl;
        std::cerr << "  " << progName << " count sensor1.out" << std::endl;
        std::cerr << "  " << progName << " count < sensor1.out" << std::endl;
        std::cerr << "  " << progName << " count -r -e .out /path/to/logs" << std::endl;
        std::cerr << "  " << progName << " count --remove-errors sensor1.out" << std::endl;
        std::cerr << "  " << progName << " count --only-value type:temperature sensor1.out" << std::endl;
    }

public:
    DataCounter(int argc, char* argv[]) : hasInputFiles(false), recursive(false), 
        extensionFilter(""), maxDepth(-1), verbosity(0), removeErrors(false), 
        removeEmptyJson(false), inputFormat("json"), minDate(0), maxDate(0) {
        
        // Check for help flag first
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--help" || arg == "-h") {
                printCountUsage(argv[0]);
                exit(0);
            }
        }

        // Parse arguments
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "-r" || arg == "--recursive") {
                recursive = true;
            } else if (arg == "-v") {
                verbosity = 1;
            } else if (arg == "-V") {
                verbosity = 2;
            } else if (arg == "--remove-errors") {
                removeErrors = true;
            } else if (arg == "--remove-empty-json") {
                removeEmptyJson = true;
            } else if (arg == "-f" || arg == "--format") {
                if (i + 1 < argc) {
                    ++i;
                    inputFormat = argv[i];
                    std::transform(inputFormat.begin(), inputFormat.end(), inputFormat.begin(), ::tolower);
                    if (inputFormat != "json" && inputFormat != "csv") {
                        std::cerr << "Error: format must be 'json' or 'csv'" << std::endl;
                        exit(1);
                    }
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    exit(1);
                }
            } else if (arg == "-e" || arg == "--extension") {
                if (i + 1 < argc) {
                    ++i;
                    extensionFilter = argv[i];
                    if (!extensionFilter.empty() && extensionFilter[0] != '.') {
                        extensionFilter = "." + extensionFilter;
                    }
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    exit(1);
                }
            } else if (arg == "-d" || arg == "--depth") {
                if (i + 1 < argc) {
                    ++i;
                    try {
                        maxDepth = std::stoi(argv[i]);
                        if (maxDepth < 0) {
                            std::cerr << "Error: depth must be non-negative" << std::endl;
                            exit(1);
                        }
                    } catch (...) {
                        std::cerr << "Error: invalid depth value" << std::endl;
                        exit(1);
                    }
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    exit(1);
                }
            } else if (arg == "--not-empty") {
                if (i + 1 < argc) {
                    ++i;
                    notEmptyColumns.insert(argv[i]);
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    exit(1);
                }
            } else if (arg == "--only-value") {
                if (i + 1 < argc) {
                    ++i;
                    std::string filter = argv[i];
                    size_t colonPos = filter.find(':');
                    if (colonPos == std::string::npos || colonPos == 0 || colonPos == filter.length() - 1) {
                        std::cerr << "Error: --only-value requires format 'column:value'" << std::endl;
                        exit(1);
                    }
                    std::string column = filter.substr(0, colonPos);
                    std::string value = filter.substr(colonPos + 1);
                    onlyValueFilters[column].insert(value);
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    exit(1);
                }
            } else if (arg == "--exclude-value") {
                if (i + 1 < argc) {
                    ++i;
                    std::string filter = argv[i];
                    size_t colonPos = filter.find(':');
                    if (colonPos == std::string::npos || colonPos == 0 || colonPos == filter.length() - 1) {
                        std::cerr << "Error: --exclude-value requires format 'column:value'" << std::endl;
                        exit(1);
                    }
                    std::string column = filter.substr(0, colonPos);
                    std::string value = filter.substr(colonPos + 1);
                    excludeValueFilters[column].insert(value);
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    exit(1);
                }
            } else if (arg == "--min-date") {
                if (i + 1 < argc) {
                    ++i;
                    minDate = DateUtils::parseDate(argv[i]);
                    if (minDate < 0) {
                        std::cerr << "Error: invalid date format for --min-date" << std::endl;
                        exit(1);
                    }
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    exit(1);
                }
            } else if (arg == "--max-date") {
                if (i + 1 < argc) {
                    ++i;
                    maxDate = DateUtils::parseDate(argv[i]);
                    if (maxDate < 0) {
                        std::cerr << "Error: invalid date format for --max-date" << std::endl;
                        exit(1);
                    }
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    exit(1);
                }
            } else if (arg[0] != '-') {
                // Input file or directory
                hasInputFiles = true;
                inputFiles.push_back(arg);
            } else {
                std::cerr << "Error: Unknown option '" << arg << "'" << std::endl;
                printCountUsage(argv[0]);
                exit(1);
            }
        }

        // Expand directories to file lists
        if (hasInputFiles) {
            std::vector<std::string> expandedFiles;
            for (const auto& path : inputFiles) {
                auto files = FileUtils::expandPath(path, recursive, extensionFilter, maxDepth);
                expandedFiles.insert(expandedFiles.end(), files.begin(), files.end());
            }
            inputFiles = expandedFiles;
        }
    }

    void count() {
        long long totalCount = 0;

        if (!hasInputFiles) {
            // Read from stdin
            totalCount = countFromStdin();
        } else {
            // Process input files
            for (const auto& file : inputFiles) {
                totalCount += countFromFile(file);
            }
        }

        std::cout << totalCount << std::endl;
    }
};

#endif // DATA_COUNTER_H
