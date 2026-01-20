#ifndef COMMON_ARG_PARSER_H
#define COMMON_ARG_PARSER_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include "date_utils.h"
#include "file_collector.h"
#include "reading_filter.h"  // For UpdateRule

// Default input format: "auto" detects from file extension, "json"/"csv" override
const std::string DEFAULT_INPUT_FORMAT = "auto";

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
    std::map<std::string, std::set<std::string>> onlyValueFilters;
    std::map<std::string, std::set<std::string>> excludeValueFilters;
    std::map<std::string, std::set<std::string>> allowedValues;
    std::set<std::string> notEmptyColumns;
    std::set<std::string> notNullColumns;
    bool removeEmptyJson;
    bool removeErrors;
    int tailLines;  // --tail <n>: only read last n lines from each file
    std::vector<UpdateRule> updateRules;  // --update-value and --update-where-empty
    
    // --tail-column-value column:value n: return last n rows where column=value
    std::string tailColumnValueColumn;
    std::string tailColumnValueValue;
    int tailColumnValueCount;
    
public:
    CommonArgParser() 
        : recursive(false), extensionFilter(""), maxDepth(-1), verbosity(0), 
          inputFormat(DEFAULT_INPUT_FORMAT), minDate(0), maxDate(0), removeEmptyJson(false), removeErrors(false),
          tailLines(0), tailColumnValueCount(0) {}
    
    // Parse common arguments and collect files
    // Returns true if parsing should continue, false if help was shown or error occurred
    bool parse(int argc, char* argv[]) {
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
            } else if (arg == "-if" || arg == "--input-format") {
                if (i + 1 < argc) {
                    ++i;
                    inputFormat = argv[i];
                    std::transform(inputFormat.begin(), inputFormat.end(), inputFormat.begin(), ::tolower);
                    if (inputFormat != "json" && inputFormat != "csv") {
                        std::cerr << "Error: input format must be 'json' or 'csv'" << std::endl;
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
            } else if (arg == "-of" || arg == "--output-format") {
                // Skip this flag and its argument - handled by SensorDataTransformer
                if (i + 1 < argc) {
                    ++i;
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    return false;
                }
            } else if (arg == "-f" || arg == "--follow") {
                // Skip this flag - handled by DataCounter and StatsAnalyser
            } else if (arg == "--tail") {
                if (i + 1 < argc) {
                    ++i;
                    try {
                        tailLines = std::stoi(argv[i]);
                        if (tailLines <= 0) {
                            std::cerr << "Error: --tail requires a positive number" << std::endl;
                            return false;
                        }
                    } catch (...) {
                        std::cerr << "Error: invalid value for --tail: " << argv[i] << std::endl;
                        return false;
                    }
                } else {
                    std::cerr << "Error: --tail requires a number argument" << std::endl;
                    return false;
                }
            } else if (arg == "--tail-column-value") {
                // --tail-column-value column:value n - return last n rows where column=value
                if (i + 2 < argc) {
                    ++i;
                    std::string colVal = argv[i];
                    size_t colonPos = colVal.find(':');
                    if (colonPos == std::string::npos || colonPos == 0 || colonPos == colVal.length() - 1) {
                        std::cerr << "Error: --tail-column-value requires format 'column:value n'" << std::endl;
                        return false;
                    }
                    tailColumnValueColumn = colVal.substr(0, colonPos);
                    tailColumnValueValue = colVal.substr(colonPos + 1);
                    ++i;
                    try {
                        tailColumnValueCount = std::stoi(argv[i]);
                        if (tailColumnValueCount <= 0) {
                            std::cerr << "Error: --tail-column-value count must be a positive number" << std::endl;
                            return false;
                        }
                    } catch (...) {
                        std::cerr << "Error: invalid count for --tail-column-value: " << argv[i] << std::endl;
                        return false;
                    }
                } else {
                    std::cerr << "Error: --tail-column-value requires 'column:value n'" << std::endl;
                    return false;
                }
            } else if (arg == "-o" || arg == "--output") {
                // Skip this flag and its argument - handled by SensorDataTransformer
                if (i + 1 < argc) {
                    ++i;
                }
            } else if (arg == "--not-empty") {
                if (i + 1 < argc) {
                    ++i;
                    notEmptyColumns.insert(argv[i]);
                } else {
                    std::cerr << "Error: --not-empty requires an argument" << std::endl;
                    return false;
                }
            } else if (arg == "--not-null") {
                if (i + 1 < argc) {
                    ++i;
                    notNullColumns.insert(argv[i]);
                } else {
                    std::cerr << "Error: --not-null requires an argument" << std::endl;
                    return false;
                }
            } else if (arg == "--remove-empty-json") {
                removeEmptyJson = true;
            } else if (arg == "--remove-errors") {
                removeErrors = true;
            } else if (arg == "--clean") {
                // --clean expands to --remove-empty-json --not-empty value --remove-errors --not-null value --not-null sensor_id
                removeEmptyJson = true;
                notEmptyColumns.insert("value");
                removeErrors = true;
                notNullColumns.insert("value");
                notNullColumns.insert("sensor_id");
            } else if (arg == "--only-value") {
                if (i + 1 < argc) {
                    ++i;
                    std::string value = argv[i];
                    size_t colonPos = value.find(':');
                    if (colonPos == std::string::npos || colonPos == 0 || colonPos == value.length() - 1) {
                        std::cerr << "Error: --only-value requires format 'column:value'" << std::endl;
                        return false;
                    }
                    std::string col = value.substr(0, colonPos);
                    std::string val = value.substr(colonPos + 1);
                    onlyValueFilters[col].insert(val);
                } else {
                    std::cerr << "Error: --only-value requires an argument" << std::endl;
                    return false;
                }
            } else if (arg == "--exclude-value") {
                if (i + 1 < argc) {
                    ++i;
                    std::string value = argv[i];
                    size_t colonPos = value.find(':');
                    if (colonPos == std::string::npos || colonPos == 0 || colonPos == value.length() - 1) {
                        std::cerr << "Error: --exclude-value requires format 'column:value'" << std::endl;
                        return false;
                    }
                    std::string col = value.substr(0, colonPos);
                    std::string val = value.substr(colonPos + 1);
                    excludeValueFilters[col].insert(val);
                } else {
                    std::cerr << "Error: --exclude-value requires an argument" << std::endl;
                    return false;
                }
            } else if (arg == "--allowed-values") {
                if (i + 2 < argc) {
                    ++i;
                    std::string column = argv[i];
                    ++i;
                    std::string valuesArg = argv[i];
                    // Check if valuesArg is a file or comma-separated values
                    std::ifstream file(valuesArg);
                    if (file.good()) {
                        // It's a file - read values line by line
                        std::string line;
                        line.reserve(256); // Reserve space to avoid frequent reallocations
                        while (std::getline(file, line)) {
                            // Trim whitespace
                            size_t start = line.find_first_not_of(" \t\r\n");
                            if (start != std::string::npos) {
                                size_t end = line.find_last_not_of(" \t\r\n");
                                if (end != std::string::npos && end >= start) {
                                    allowedValues[column].insert(line.substr(start, end - start + 1));
                                }
                            }
                            line.clear(); // Explicitly clear for next iteration
                        }
                        file.close(); // Explicitly close the file
                    } else {
                        // Treat as comma-separated values
                        size_t pos = 0;
                        while (pos < valuesArg.length()) {
                            size_t commaPos = valuesArg.find(',', pos);
                            if (commaPos == std::string::npos) {
                                commaPos = valuesArg.length();
                            }
                            std::string val = valuesArg.substr(pos, commaPos - pos);
                            // Trim whitespace
                            size_t start = val.find_first_not_of(" \t");
                            size_t end = val.find_last_not_of(" \t");
                            if (start != std::string::npos && end != std::string::npos) {
                                allowedValues[column].insert(val.substr(start, end - start + 1));
                            }
                            pos = commaPos + 1;
                        }
                    }
                    if (allowedValues[column].empty()) {
                        std::cerr << "Error: --allowed-values requires at least one value" << std::endl;
                        return false;
                    }
                } else {
                    std::cerr << "Error: --allowed-values requires <column> and <values|file>" << std::endl;
                    return false;
                }
            } else if (arg == "-c" || arg == "--column") {
                // Skip this flag and its argument - handled by StatsAnalyser
                if (i + 1 < argc) {
                    ++i;
                }
            } else if (arg == "--use-prototype" || arg == "--remove-errors" || arg == "--remove-whitespace" || arg == "--remove-empty-json") {
                // Skip these flags - handled by SensorDataTransformer
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
            } else if (arg == "--update-value" || arg == "--update-where-empty") {
                // Format: --update-value col1:existing_value col2:new_value
                // Sets col2 to new_value when col1 == existing_value
                bool onlyWhenEmpty = (arg == "--update-where-empty");
                if (i + 2 < argc) {
                    ++i;
                    std::string matchArg = argv[i];
                    ++i;
                    std::string targetArg = argv[i];
                    
                    // Parse matchArg as "column:value"
                    size_t colonPos1 = matchArg.find(':');
                    if (colonPos1 == std::string::npos || colonPos1 == 0 || colonPos1 == matchArg.length() - 1) {
                        std::cerr << "Error: " << arg << " first argument must be 'column:value'" << std::endl;
                        return false;
                    }
                    std::string matchCol = matchArg.substr(0, colonPos1);
                    std::string matchVal = matchArg.substr(colonPos1 + 1);
                    
                    // Parse targetArg as "column:value"
                    size_t colonPos2 = targetArg.find(':');
                    if (colonPos2 == std::string::npos || colonPos2 == 0 || colonPos2 == targetArg.length() - 1) {
                        std::cerr << "Error: " << arg << " second argument must be 'column:value'" << std::endl;
                        return false;
                    }
                    std::string targetCol = targetArg.substr(0, colonPos2);
                    std::string newVal = targetArg.substr(colonPos2 + 1);
                    
                    updateRules.emplace_back(matchCol, matchVal, targetCol, newVal, onlyWhenEmpty);
                } else {
                    std::cerr << "Error: " << arg << " requires two arguments: 'match_col:match_val' 'target_col:new_val'" << std::endl;
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
        
        inputFiles = collector.getSortedFiles();
        return true;
    }
    
    // Getters
    bool getRecursive() const noexcept { return recursive; }
    const std::string& getExtensionFilter() const noexcept { return extensionFilter; }
    int getMaxDepth() const noexcept { return maxDepth; }
    int getVerbosity() const noexcept { return verbosity; }
    const std::string& getInputFormat() const noexcept { return inputFormat; }
    long long getMinDate() const noexcept { return minDate; }
    long long getMaxDate() const noexcept { return maxDate; }
    const std::vector<std::string>& getInputFiles() const noexcept { return inputFiles; }
    const std::map<std::string, std::set<std::string>>& getOnlyValueFilters() const noexcept { return onlyValueFilters; }
    const std::map<std::string, std::set<std::string>>& getExcludeValueFilters() const noexcept { return excludeValueFilters; }
    const std::map<std::string, std::set<std::string>>& getAllowedValues() const noexcept { return allowedValues; }
    const std::set<std::string>& getNotEmptyColumns() const noexcept { return notEmptyColumns; }
    const std::set<std::string>& getNotNullColumns() const noexcept { return notNullColumns; }
    bool getRemoveEmptyJson() const noexcept { return removeEmptyJson; }
    bool getRemoveErrors() const noexcept { return removeErrors; }
    int getTailLines() const noexcept { return tailLines; }
    const std::vector<UpdateRule>& getUpdateRules() const noexcept { return updateRules; }
    const std::string& getTailColumnValueColumn() const noexcept { return tailColumnValueColumn; }
    const std::string& getTailColumnValueValue() const noexcept { return tailColumnValueValue; }
    int getTailColumnValueCount() const noexcept { return tailColumnValueCount; }
    
    /**
     * Check for unknown options in command line arguments.
     * Returns empty string if all options are valid, or the unknown option if found.
     * 
     * @param argc Argument count
     * @param argv Argument values
     * @param additionalAllowed Set of additional allowed options for this command
     */
    static std::string checkUnknownOptions(int argc, char* argv[], 
                                           const std::set<std::string>& additionalAllowed = {}) {
        // Common options allowed by all commands
        static const std::set<std::string> commonOptions = {
            "-r", "--recursive", "-v", "-V", "-if", "--input-format",
            "-e", "--extension", "-d", "--depth", "--min-date", "--max-date",
            "--tail", "--tail-column-value", "-h", "--help"
        };
        
        // Common filtering options
        static const std::set<std::string> filterOptions = {
            "--not-empty", "--not-null", "--only-value", "--exclude-value", "--allowed-values",
            "--remove-errors", "--remove-empty-json", "--clean",
            "--update-value", "--update-where-empty"
        };
        
        // Options that take arguments (need to skip the next arg)
        // Note: --allowed-values and --tail-column-value take TWO args but we handle that specially
        static const std::set<std::string> optionsWithArgs = {
            "-if", "--input-format", "-e", "--extension", "-d", "--depth",
            "--min-date", "--max-date", "--not-empty", "--not-null", "--only-value", 
            "--exclude-value", "--allowed-values", "-o", "--output", "-of", "--output-format",
            "-c", "--column", "--tail", "--tail-column-value"
        };
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            // Skip non-option arguments
            if (arg.empty() || arg[0] != '-') {
                continue;
            }
            
            // Check if previous arg was an option that takes an argument
            if (i > 1) {
                std::string prev = argv[i-1];
                if (optionsWithArgs.count(prev) > 0 || additionalAllowed.count(prev) > 0) {
                    continue;  // This is an argument to the previous option
                }
            }
            
            // Check if this is an allowed option
            if (commonOptions.count(arg) > 0 || 
                filterOptions.count(arg) > 0 || 
                additionalAllowed.count(arg) > 0) {
                continue;
            }
            
            // Unknown option found
            return arg;
        }
        
        return "";  // All options are valid
    }
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
