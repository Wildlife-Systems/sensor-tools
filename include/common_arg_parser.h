#ifndef COMMON_ARG_PARSER_H
#define COMMON_ARG_PARSER_H

#include <iostream>
#include <string>
#include <algorithm>
#include "date_utils.h"
#include "file_collector.h"

// Centralized argument parser for common flags
class CommonArgParser {
private:
    bool recursive;
    std::string extensionFilter;
    int maxDepth;
    int verbosity;
    std::string inputFormat;
    long long minDate;
    long long maxDate;
    std::vector<std::string> inputFiles;
    
public:
    CommonArgParser() 
        : recursive(false), extensionFilter(""), maxDepth(-1), verbosity(0), 
          inputFormat("json"), minDate(0), maxDate(0) {}
    
    // Parse common arguments and collect files
    // Returns true if parsing should continue, false if help was shown or error occurred
    bool parse(int argc, char* argv[], const char* usageFunc(const char*) = nullptr) {
        FileCollector collector;
        bool collectorInitialized = false;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "-r" || arg == "--recursive") {
                recursive = true;
            } else if (arg == "-v") {
                verbosity = 1;
            } else if (arg == "-V") {
                verbosity = 2;
            } else if (arg == "-f" || arg == "--format") {
                if (i + 1 < argc) {
                    ++i;
                    inputFormat = argv[i];
                    std::transform(inputFormat.begin(), inputFormat.end(), inputFormat.begin(), ::tolower);
                    if (inputFormat != "json" && inputFormat != "csv") {
                        std::cerr << "Error: format must be 'json' or 'csv'" << std::endl;
                        return false;
                    }
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    return false;
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
                    return false;
                }
            } else if (arg == "-F" || arg == "--output-format") {
                // Skip this flag and its argument - handled by SensorDataConverter
                if (i + 1 < argc) {
                    ++i;
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    return false;
                }
            } else if (arg == "-o" || arg == "--output") {
                // Skip this flag and its argument - handled by SensorDataConverter
                if (i + 1 < argc) {
                    ++i;
                }
            } else if (arg == "--not-empty" || arg == "--only-value") {
                // Skip these flags and their arguments - handled by SensorDataConverter
                if (i + 1 < argc) {
                    ++i;
                }
            } else if (arg == "-c" || arg == "--column") {
                // Skip this flag and its argument - handled by StatsAnalyser
                if (i + 1 < argc) {
                    ++i;
                }
            } else if (arg == "--use-prototype" || arg == "--remove-errors") {
                // Skip these flags - handled by SensorDataConverter
            } else if (arg == "-d" || arg == "--depth") {
                if (i + 1 < argc) {
                    ++i;
                    try {
                        maxDepth = std::stoi(argv[i]);
                        if (maxDepth < 0) {
                            std::cerr << "Error: depth must be non-negative" << std::endl;
                            return false;
                        }
                    } catch (...) {
                        std::cerr << "Error: invalid depth value '" << argv[i] << "'" << std::endl;
                        return false;
                    }
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    return false;
                }
            } else if (arg == "--min-date") {
                if (i + 1 < argc) {
                    ++i;
                    minDate = DateUtils::parseDate(argv[i]);
                    if (minDate == 0) {
                        std::cerr << "Error: invalid date format for --min-date" << std::endl;
                        return false;
                    }
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    return false;
                }
            } else if (arg == "--max-date") {
                if (i + 1 < argc) {
                    ++i;
                    maxDate = DateUtils::parseDateEndOfDay(argv[i]);
                    if (maxDate == 0) {
                        std::cerr << "Error: invalid date format for --max-date" << std::endl;
                        return false;
                    }
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    return false;
                }
            } else if (arg[0] != '-') {
                // It's a file or directory path
                if (!collectorInitialized) {
                    collector = FileCollector(recursive, extensionFilter, maxDepth, verbosity);
                    collectorInitialized = true;
                }
                collector.addPath(arg);
            }
            // Unknown flags are ignored here - let each class handle their specific flags
        }
        
        inputFiles = collector.getFiles();
        return true;
    }
    
    // Getters
    bool getRecursive() const { return recursive; }
    const std::string& getExtensionFilter() const { return extensionFilter; }
    int getMaxDepth() const { return maxDepth; }
    int getVerbosity() const { return verbosity; }
    const std::string& getInputFormat() const { return inputFormat; }
    long long getMinDate() const { return minDate; }
    long long getMaxDate() const { return maxDate; }
    const std::vector<std::string>& getInputFiles() const { return inputFiles; }
};

// Helper function to print common verbose information
inline void printCommonVerboseInfo(const std::string& toolName, int verbosity, bool recursive, 
                                   const std::string& extensionFilter, int maxDepth, size_t fileCount) {
    if (verbosity >= 1) {
        std::cout << toolName << " with verbosity level " << verbosity << std::endl;
        std::cout << "Recursive: " << (recursive ? "yes" : "no") << std::endl;
        if (!extensionFilter.empty()) {
            std::cout << "Extension filter: " << extensionFilter << std::endl;
        }
        if (maxDepth >= 0) {
            std::cout << "Max depth: " << maxDepth << std::endl;
        }
        std::cout << "Processing " << fileCount << " file(s)..." << std::endl;
    }
}

#endif // COMMON_ARG_PARSER_H
