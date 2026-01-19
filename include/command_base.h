#ifndef COMMAND_BASE_H
#define COMMAND_BASE_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <future>
#include <mutex>

#include "date_utils.h"
#include "common_arg_parser.h"
#include "csv_parser.h"
#include "json_parser.h"
#include "error_detector.h"
#include "file_utils.h"
#include "file_collector.h"

/**
 * Base class for command handlers providing shared functionality:
 * - Common command-line options parsing
 * - Date range filtering
 * - Value-based filtering (include/exclude)
 * - Error reading detection
 * - File/stdin processing
 */
class CommandBase {
protected:
    // Common options
    std::vector<std::string> inputFiles;
    bool hasInputFiles;
    bool recursive;
    std::string extensionFilter;
    int maxDepth;
    int verbosity;
    std::string inputFormat;
    long long minDate;
    long long maxDate;
    
    // Filtering options
    bool removeErrors;
    bool removeEmptyJson;
    std::set<std::string> notEmptyColumns;
    std::set<std::string> notNullColumns;
    std::map<std::string, std::set<std::string>> onlyValueFilters;
    std::map<std::string, std::set<std::string>> excludeValueFilters;
    std::map<std::string, std::set<std::string>> allowedValues;
    
    // Performance options
    int tailLines;  // --tail <n>: only read last n lines from each file (0 = read all)
    
    // Constructor with default values
    CommandBase() 
        : hasInputFiles(false)
        , recursive(false)
        , extensionFilter("")
        , maxDepth(-1)
        , verbosity(0)
        , inputFormat(DEFAULT_INPUT_FORMAT)
        , minDate(0)
        , maxDate(0)
        , removeErrors(false)
        , removeEmptyJson(false)
        , tailLines(0) {}
    
    virtual ~CommandBase() = default;
    
    /**
     * Check if a reading passes the date filter
     */
    bool passesDateFilter(const std::map<std::string, std::string>& reading) const {
        if (minDate > 0 || maxDate > 0) {
            long long timestamp = DateUtils::getTimestamp(reading);
            return DateUtils::isInDateRange(timestamp, minDate, maxDate);
        }
        return true;
    }
    
    /**
     * Check if all readings in a vector are empty (for removeEmptyJson filtering)
     */
    static bool areAllReadingsEmpty(const std::vector<std::map<std::string, std::string>>& readings) {
        for (const auto& r : readings) {
            if (!r.empty()) return false;
        }
        return true;
    }
    
    /**
     * Check if a reading should be included based on all active filters.
     * Subclasses can override for additional filtering.
     */
    virtual bool shouldIncludeReading(const std::map<std::string, std::string>& reading) {
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
                // Check for literal "null" string or ASCII null character
                bool hasNullChar = false;
                for (size_t i = 0; i < val.length(); ++i) {
                    if (val[i] == '\0') {
                        hasNullChar = true;
                        break;
                    }
                }
                if (val == "null" || hasNullChar) {
                    if (verbosity >= 2) {
                        std::cerr << "  Skipping row: null value in column '" << reqCol << "'" << std::endl;
                    }
                    return false;
                }
            }
        }
        
        // Check value filters (include)
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
     * Copy common options from the CommonArgParser
     */
    void copyFromParser(const CommonArgParser& parser) {
        recursive = parser.getRecursive();
        extensionFilter = parser.getExtensionFilter();
        maxDepth = parser.getMaxDepth();
        verbosity = parser.getVerbosity();
        inputFormat = parser.getInputFormat();
        minDate = parser.getMinDate();
        maxDate = parser.getMaxDate();
        inputFiles = parser.getInputFiles();
        hasInputFiles = !inputFiles.empty();
        onlyValueFilters = parser.getOnlyValueFilters();
        excludeValueFilters = parser.getExcludeValueFilters();
        allowedValues = parser.getAllowedValues();
        notEmptyColumns = parser.getNotEmptyColumns();
        notNullColumns = parser.getNotNullColumns();
        removeEmptyJson = parser.getRemoveEmptyJson();
        removeErrors = parser.getRemoveErrors();
        tailLines = parser.getTailLines();
    }
    
    /**
     * Print verbose filter information
     */
    void printFilterInfo() const {
        if (verbosity >= 1) {
            if (!notEmptyColumns.empty()) {
                std::cerr << "Required non-empty columns: ";
                bool first = true;
                for (const auto& col : notEmptyColumns) {
                    if (!first) std::cerr << ", ";
                    std::cerr << col;
                    first = false;
                }
                std::cerr << std::endl;
            }
            if (!onlyValueFilters.empty()) {
                std::cerr << "Value filters (include): ";
                bool first = true;
                for (const auto& filter : onlyValueFilters) {
                    for (const auto& val : filter.second) {
                        if (!first) std::cerr << ", ";
                        std::cerr << filter.first << "=" << val;
                        first = false;
                    }
                }
                std::cerr << std::endl;
            }
            if (!excludeValueFilters.empty()) {
                std::cerr << "Value filters (exclude): ";
                bool first = true;
                for (const auto& filter : excludeValueFilters) {
                    for (const auto& val : filter.second) {
                        if (!first) std::cerr << ", ";
                        std::cerr << filter.first << "=" << val;
                        first = false;
                    }
                }
                std::cerr << std::endl;
            }
            if (!allowedValues.empty()) {
                for (const auto& filter : allowedValues) {
                    std::cerr << "Allowed values for '" << filter.first << "': " 
                              << filter.second.size() << " value(s)" << std::endl;
                }
            }
        }
    }
    
    // ===== CSV/JSON Writing Utilities =====
    
    /**
     * Write a single row to CSV output
     */
    static void writeCsvRow(const std::map<std::string, std::string>& reading,
                            const std::vector<std::string>& headers,
                            std::ostream& outfile) {
        for (size_t i = 0; i < headers.size(); ++i) {
            if (i > 0) outfile << ",";
            
            auto it = reading.find(headers[i]);
            if (it != reading.end()) {
                std::string value = it->second;
                // Escape quotes and wrap in quotes if contains comma or quote
                if (value.find(',') != std::string::npos || 
                    value.find('"') != std::string::npos ||
                    value.find('\n') != std::string::npos) {
                    // Escape quotes
                    size_t pos = 0;
                    while ((pos = value.find('"', pos)) != std::string::npos) {
                        value.replace(pos, 1, "\"\"");
                        pos += 2;
                    }
                    outfile << "\"" << value << "\"";
                } else {
                    outfile << value;
                }
            }
        }
        outfile << "\n";
    }
    
    /**
     * Escape a string for JSON output
     */
    static std::string escapeJsonString(const std::string& str) {
        std::string result;
        result.reserve(str.size() + 10);
        for (char c : str) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }
    
    /**
     * Check if a string is a valid JSON number
     */
    static bool isJsonNumber(const std::string& str) {
        if (str.empty()) return false;
        size_t i = 0;
        if (str[i] == '-') ++i;
        if (i >= str.size()) return false;
        if (str[i] == '0') {
            ++i;
        } else if (str[i] >= '1' && str[i] <= '9') {
            while (i < str.size() && str[i] >= '0' && str[i] <= '9') ++i;
        } else {
            return false;
        }
        if (i < str.size() && str[i] == '.') {
            ++i;
            if (i >= str.size() || str[i] < '0' || str[i] > '9') return false;
            while (i < str.size() && str[i] >= '0' && str[i] <= '9') ++i;
        }
        if (i < str.size() && (str[i] == 'e' || str[i] == 'E')) {
            ++i;
            if (i < str.size() && (str[i] == '+' || str[i] == '-')) ++i;
            if (i >= str.size() || str[i] < '0' || str[i] > '9') return false;
            while (i < str.size() && str[i] >= '0' && str[i] <= '9') ++i;
        }
        return i == str.size();
    }
    
    /**
     * Write a single reading as JSON object
     */
    static void writeJsonObject(const std::map<std::string, std::string>& reading,
                                std::ostream& outfile, bool compact = false) {
        const char* sp = compact ? "" : " ";
        outfile << "{" << sp;
        bool first = true;
        for (const auto& pair : reading) {
            if (!first) outfile << "," << sp;
            first = false;
            outfile << "\"" << escapeJsonString(pair.first) << "\":" << sp;
            
            // Check if value is a number, boolean, or null
            const std::string& val = pair.second;
            if (val == "null" || val.empty()) {
                outfile << "null";
            } else if (val == "true" || val == "false") {
                outfile << val;
            } else if (isJsonNumber(val)) {
                outfile << val;
            } else {
                outfile << "\"" << escapeJsonString(val) << "\"";
            }
        }
        outfile << sp << "}";
    }
    
    /**
     * Process files in parallel using multiple threads.
     * @param files Vector of file paths to process
     * @param processFunc Function to call for each file (takes filename, returns result of type T)
     * @param combineFunc Function to combine results (takes accumulator ref and result, modifies accumulator in-place)
     * @param initialValue Initial value for the accumulator
     * @param numThreads Number of threads to use (default: 4)
     * @return Combined result
     */
    template<typename T, typename ProcessFunc, typename CombineFunc>
    static T processFilesParallel(const std::vector<std::string>& files,
                                   ProcessFunc processFunc,
                                   CombineFunc combineFunc,
                                   T initialValue,
                                   size_t numThreads = 4) {
        if (files.empty()) {
            return initialValue;
        }
        
        // For small number of files, process sequentially
        if (files.size() <= 2 || numThreads <= 1) {
            T result = initialValue;
            for (const auto& file : files) {
                combineFunc(result, processFunc(file));
            }
            return result;
        }
        
        std::mutex resultMutex;
        T combinedResult = initialValue;
        
        size_t filesPerThread = std::max(size_t(1), files.size() / numThreads);
        std::vector<std::future<void>> futures;
        futures.reserve(numThreads);
        
        for (size_t i = 0; i < files.size(); i += filesPerThread) {
            size_t end = std::min(i + filesPerThread, files.size());
            
            futures.push_back(std::async(std::launch::async, [&, i, end]() {
                T localResult = initialValue;
                for (size_t j = i; j < end; ++j) {
                    combineFunc(localResult, processFunc(files[j]));
                }
                
                std::lock_guard<std::mutex> lock(resultMutex);
                combineFunc(combinedResult, localResult);
            }));
        }
        
        for (auto& f : futures) {
            f.wait();
        }
        
        return combinedResult;
    }
    
    /**
     * Process files in parallel, calling a void function for each file.
     * @param files Vector of file paths to process
     * @param processFunc Function to call for each file (takes filename)
     * @param numThreads Number of threads to use (default: 4)
     */
    template<typename ProcessFunc>
    static void processFilesParallelVoid(const std::vector<std::string>& files,
                                          ProcessFunc processFunc,
                                          size_t numThreads = 4) {
        if (files.empty()) {
            return;
        }
        
        // For small number of files, process sequentially
        if (files.size() <= 2 || numThreads <= 1) {
            for (const auto& file : files) {
                processFunc(file);
            }
            return;
        }
        
        size_t filesPerThread = std::max(size_t(1), files.size() / numThreads);
        std::vector<std::future<void>> futures;
        futures.reserve(numThreads);
        
        for (size_t i = 0; i < files.size(); i += filesPerThread) {
            size_t end = std::min(i + filesPerThread, files.size());
            
            futures.push_back(std::async(std::launch::async, [&, i, end]() {
                for (size_t j = i; j < end; ++j) {
                    processFunc(files[j]);
                }
            }));
        }
        
        for (auto& f : futures) {
            f.wait();
        }
    }
};

#endif // COMMAND_BASE_H
