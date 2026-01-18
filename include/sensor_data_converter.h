#ifndef SENSOR_DATA_CONVERTER_H
#define SENSOR_DATA_CONVERTER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <thread>
#include <mutex>
#include <future>
#include <cstdio>
#include <cstdlib>
#include <array>

#include "date_utils.h"
#include "common_arg_parser.h"
#include "csv_parser.h"
#include "json_parser.h"
#include "error_detector.h"
#include "file_utils.h"

class SensorDataConverter {
private:
    std::vector<std::string> inputFiles;
    std::string outputFile;
    std::set<std::string> allKeys;
    std::mutex keysMutex;  // For thread-safe key collection
    bool hasInputFiles;
    bool recursive;
    std::string extensionFilter;
    int maxDepth;
    int numThreads;
    bool usePrototype;
    std::set<std::string> notEmptyColumns;  // Columns that must not be empty
    std::map<std::string, std::set<std::string>> onlyValueFilters;  // Column:value pairs for filtering (include)
    std::map<std::string, std::set<std::string>> excludeValueFilters;  // Column:value pairs for filtering (exclude)
    int verbosity;  // 0 = normal, 1 = verbose (-v), 2 = very verbose (-V)
    bool removeErrors;  // Remove error readings (e.g., DS18B20 temperature = 85)
    bool removeWhitespace;  // Remove extra whitespace from output (compact format)
    std::string inputFormat;  // Input format: "json" or "csv" (default: json for stdin)
    std::string outputFormat;  // Output format: "json" or "csv" (default: json for stdout, csv for file)
    long long minDate;  // Minimum timestamp (Unix epoch)
    long long maxDate;  // Maximum timestamp (Unix epoch)
    
    // Check if any filtering is active (affects whether we can pass-through JSON lines)
    bool hasActiveFilters() const {
        return !notEmptyColumns.empty() || !onlyValueFilters.empty() || 
               !excludeValueFilters.empty() || removeErrors || removeWhitespace || minDate > 0 || maxDate > 0;
    }
    
    // Execute sc-prototype command and parse columns from JSON output
    bool getPrototypeColumns() {
        std::array<char, 128> buffer;
        std::string result;
        
        // Execute sc-prototype command
        FILE* pipe = popen("sc-prototype", "r");
        
        if (!pipe) {
            std::cerr << "Error: Failed to run sc-prototype command" << std::endl;
            return false;
        }
        
        // Read output
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
        
        // Close pipe
        pclose(pipe);
        
        if (result.empty()) {
            std::cerr << "Error: sc-prototype returned no output" << std::endl;
            return false;
        }
        
        // Parse JSON to extract keys
        auto readings = JsonParser::parseJsonLine(result);
        if (readings.empty() || readings[0].empty()) {
            std::cerr << "Error: Failed to parse sc-prototype output" << std::endl;
            return false;
        }
        
        // Extract all keys from the prototype
        for (const auto& pair : readings[0]) {
            allKeys.insert(pair.first);
        }
        
        std::cout << "Loaded " << allKeys.size() << " columns from sc-prototype" << std::endl;
        return true;
    }
    
    // First pass: collect all column names from a file (thread-safe)
    void collectKeysFromFile(const std::string& filename) {
        if (verbosity >= 2) {
            std::cout << "Collecting keys from: " << filename << std::endl;
        }
        
        std::ifstream infile(filename);
        if (!infile) {
            std::cerr << "Warning: Cannot open file: " << filename << std::endl;
            return;
        }
        
        std::set<std::string> localKeys;  // Thread-local key collection
        std::string line;
        
        // Check if this is a CSV file
        if (FileUtils::isCsvFile(filename)) {
            // CSV format - first line is header
            if (std::getline(infile, line) && !line.empty()) {
                auto fields = CsvParser::parseCsvLine(line);
                for (const auto& field : fields) {
                    if (!field.empty()) {
                        localKeys.insert(field);
                    }
                }
            }
        } else {
            // JSON format
            while (std::getline(infile, line)) {
                if (line.empty()) continue;
                
                auto readings = JsonParser::parseJsonLine(line);
                for (const auto& reading : readings) {
                    for (const auto& pair : reading) {
                        localKeys.insert(pair.first);
                    }
                }
            }
        }
        
        // Merge into global keys with mutex
        {
            std::lock_guard<std::mutex> lock(keysMutex);
            allKeys.insert(localKeys.begin(), localKeys.end());
            if (verbosity >= 2) {
                std::cout << "  Collected " << allKeys.size() << " unique keys so far" << std::endl;
            }
        }
        
        infile.close();
    }
    
    // Second pass: write rows from a file directly to CSV (not thread-safe, called sequentially)
    void writeRowsFromFile(const std::string& filename, std::ostream& outfile, 
                           const std::vector<std::string>& headers) {
        if (verbosity >= 1) {
            std::cout << "Processing file: " << filename << std::endl;
        }
        
        std::ifstream infile(filename);
        if (!infile) {
            std::cerr << "Warning: Cannot open file: " << filename << std::endl;
            return;
        }
        
        std::string line;
        
        // Check if this is a CSV file
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
                
                // Apply filtering
                if (!shouldIncludeReading(reading)) continue;
                
                // Write row with unified headers
                writeRow(reading, headers, outfile);
            }
        } else {
            // JSON format
            while (std::getline(infile, line)) {
                if (line.empty()) continue;
                
                auto readings = JsonParser::parseJsonLine(line);
                for (const auto& reading : readings) {
                    if (reading.empty()) continue;
                    
                    // Apply filtering
                    if (!shouldIncludeReading(reading)) continue;
                    
                    // Write row
                    writeRow(reading, headers, outfile);
                }
            }
        }
        
        infile.close();
    }
    
    // Write rows from a file as JSON - preserves line-by-line array format of .out files
    void writeRowsFromFileJson(const std::string& filename, std::ostream& outfile, 
                               bool& firstOutput) {
        if (verbosity >= 1) {
            std::cerr << "Processing file: " << filename << std::endl;
        }
        
        std::ifstream infile(filename);
        if (!infile) {
            std::cerr << "Warning: Cannot open file: " << filename << std::endl;
            return;
        }
        
        std::string line;
        const char* sp = removeWhitespace ? "" : " ";
        
        // Check if this is a CSV file
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
                
                // Apply filtering
                if (!shouldIncludeReading(reading)) continue;
                
                // Write JSON array with single object (one line per reading for CSV)
                if (!firstOutput) outfile << "\n";
                firstOutput = false;
                outfile << "[" << sp;
                writeJsonObject(reading, outfile);
                outfile << sp << "]";
            }
        } else {
            // JSON format - preserve line-by-line structure
            while (std::getline(infile, line)) {
                if (line.empty()) continue;
                
                // If no filtering, pass through the original line
                if (!hasActiveFilters()) {
                    if (!firstOutput) outfile << "\n";
                    firstOutput = false;
                    outfile << line;
                } else {
                    // Parse, filter, and rebuild the line
                    auto readings = JsonParser::parseJsonLine(line);
                    std::vector<std::map<std::string, std::string>> filtered;
                    
                    for (const auto& reading : readings) {
                        if (reading.empty()) continue;
                        if (shouldIncludeReading(reading)) {
                            filtered.push_back(reading);
                        }
                    }
                    
                    if (!filtered.empty()) {
                        if (!firstOutput) outfile << "\n";
                        firstOutput = false;
                        outfile << "[" << sp;
                        for (size_t i = 0; i < filtered.size(); ++i) {
                            if (i > 0) outfile << "," << sp;
                            writeJsonObject(filtered[i], outfile);
                        }
                        outfile << sp << "]";
                    }
                }
            }
        }
        
        infile.close();
    }

    // Process stdin data that was cached in memory
    void processStdinData(const std::vector<std::string>& lines, 
                          const std::vector<std::string>& headers, 
                          std::ostream& outfile) {
        if (inputFormat == "csv") {
            // CSV format
            std::vector<std::string> csvHeaders;
            bool isFirstLine = true;
            
            for (const auto& line : lines) {
                if (line.empty()) continue;
                
                if (isFirstLine) {
                    csvHeaders = CsvParser::parseCsvLine(line);
                    isFirstLine = false;
                    continue;
                }
                
                auto fields = CsvParser::parseCsvLine(line);
                if (fields.empty()) continue;
                
                // Create a map from CSV headers to values
                std::map<std::string, std::string> reading;
                for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                    reading[csvHeaders[i]] = fields[i];
                }
                
                // Apply filtering
                if (!shouldIncludeReading(reading)) continue;
                
                // Write row
                writeRow(reading, headers, outfile);
            }
        } else {
            // JSON format
            for (const auto& line : lines) {
                if (line.empty()) continue;
                
                auto readings = JsonParser::parseJsonLine(line);
                for (const auto& reading : readings) {
                    if (reading.empty()) continue;
                    
                    // Apply filtering
                    if (!shouldIncludeReading(reading)) continue;
                    
                    // Write row
                    writeRow(reading, headers, outfile);
                }
            }
        }
    }
    
    // Process stdin data and output as JSON array
    void processStdinDataJson(const std::vector<std::string>& lines, 
                              std::ostream& outfile) {
        bool firstOutput = true;
        const char* sp = removeWhitespace ? "" : " ";
        
        if (inputFormat == "csv") {
            // CSV format
            std::vector<std::string> csvHeaders;
            bool isFirstLine = true;
            
            for (const auto& line : lines) {
                if (line.empty()) continue;
                
                if (isFirstLine) {
                    csvHeaders = CsvParser::parseCsvLine(line);
                    isFirstLine = false;
                    continue;
                }
                
                auto fields = CsvParser::parseCsvLine(line);
                if (fields.empty()) continue;
                
                // Create a map from CSV headers to values
                std::map<std::string, std::string> reading;
                for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                    reading[csvHeaders[i]] = fields[i];
                }
                
                // Apply filtering
                if (!shouldIncludeReading(reading)) continue;
                
                // Write JSON array with single object (one line per reading for CSV)
                if (!firstOutput) outfile << "\n";
                firstOutput = false;
                outfile << "[" << sp;
                writeJsonObject(reading, outfile);
                outfile << sp << "]";
            }
        }
        // Note: JSON stdin → JSON output uses the streaming path in convert(),
        // so this function is only called for CSV input → JSON output.
    }

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
    
    // Write a single row to CSV output
    void writeCsvRow(const std::map<std::string, std::string>& reading,
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
    
    // Escape a string for JSON output
    std::string escapeJsonString(const std::string& str) {
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
    
    // Check if a string is a valid JSON number
    bool isJsonNumber(const std::string& str) {
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
    
    // Write a single reading as JSON object
    void writeJsonObject(const std::map<std::string, std::string>& reading,
                         std::ostream& outfile) {
        const char* sp = removeWhitespace ? "" : " ";
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
    
    // Write a single row (dispatches to CSV or JSON based on outputFormat)
    void writeRow(const std::map<std::string, std::string>& reading,
                  const std::vector<std::string>& headers,
                  std::ostream& outfile) {
        if (outputFormat == "json") {
            writeJsonObject(reading, outfile);
            outfile << "\n";
        } else {
            writeCsvRow(reading, headers, outfile);
        }
    }

public:
    SensorDataConverter(int argc, char* argv[]) : hasInputFiles(false), recursive(false), extensionFilter(""), maxDepth(-1), numThreads(4), usePrototype(false), verbosity(0), removeErrors(false), removeWhitespace(false), inputFormat("json"), outputFormat(""), minDate(0), maxDate(0) {
        // Check for help flag first
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--help" || arg == "-h") {
                printConvertUsage(argv[0]);
                exit(0);
            }
        }
        
        // argc should be at least 1 for "convert": program (may read from stdin)
        if (argc < 1) {
            printConvertUsage(argv[0]);
            exit(1);
        }
        
        // Output file is empty by default (will use stdout)
        outputFile = "";
        
        // First pass: parse converter-specific flags
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "--use-prototype") {
                usePrototype = true;
            } else if (arg == "--remove-errors") {
                removeErrors = true;
            } else if (arg == "--remove-whitespace") {
                removeWhitespace = true;
            } else if (arg == "-o" || arg == "--output") {
                if (i + 1 < argc) {
                    ++i;
                    outputFile = argv[i];
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
            } else if (arg == "-F" || arg == "--output-format") {
                if (i + 1 < argc) {
                    ++i;
                    outputFormat = argv[i];
                    if (outputFormat != "json" && outputFormat != "csv") {
                        std::cerr << "Error: --output-format must be 'json' or 'csv'" << std::endl;
                        exit(1);
                    }
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    exit(1);
                }
            }
        }
        
        // Second pass: parse common flags and collect files
        CommonArgParser commonParser;
        if (!commonParser.parse(argc, argv)) {
            exit(1);
        }
        
        // Copy common values
        recursive = commonParser.getRecursive();
        extensionFilter = commonParser.getExtensionFilter();
        maxDepth = commonParser.getMaxDepth();
        verbosity = commonParser.getVerbosity();
        inputFormat = commonParser.getInputFormat();
        minDate = commonParser.getMinDate();
        maxDate = commonParser.getMaxDate();
        inputFiles = commonParser.getInputFiles();
        
        // If no input files, will read from stdin
        hasInputFiles = !inputFiles.empty();
    }
    
    void convert() {
        // Set default output format if not specified - JSON (.out format) is the default
        if (outputFormat.empty()) {
            outputFormat = "json";
        }
        
        if (inputFiles.empty()) {
            // Reading from stdin
            if (verbosity >= 1) {
                std::cerr << "Reading from stdin (format: " << inputFormat << ")..." << std::endl;
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
            }
            
            // Streaming path: JSON input -> JSON output (can stream line by line)
            if (inputFormat == "json" && outputFormat == "json") {
                std::ostream* outStream = &std::cout;
                std::ofstream outFile;
                
                if (!outputFile.empty()) {
                    outFile.open(outputFile);
                    if (!outFile) {
                        std::cerr << "Error: Cannot create output file: " << outputFile << std::endl;
                        return;
                    }
                    outStream = &outFile;
                }
                
                std::string line;
                bool firstOutput = true;
                bool hasInput = false;
                const char* sp = removeWhitespace ? "" : " ";
                while (std::getline(std::cin, line)) {
                    if (line.empty()) continue;
                    hasInput = true;
                    
                    if (!hasActiveFilters()) {
                        // No filtering - pass through directly
                        if (!firstOutput) *outStream << "\n";
                        firstOutput = false;
                        *outStream << line;
                    } else {
                        // Parse, filter, and rebuild the line
                        auto readings = JsonParser::parseJsonLine(line);
                        std::vector<std::map<std::string, std::string>> filtered;
                        
                        for (const auto& reading : readings) {
                            if (reading.empty()) continue;
                            if (shouldIncludeReading(reading)) {
                                filtered.push_back(reading);
                            }
                        }
                        
                        if (!filtered.empty()) {
                            if (!firstOutput) *outStream << "\n";
                            firstOutput = false;
                            *outStream << "[" << sp;
                            for (size_t i = 0; i < filtered.size(); ++i) {
                                if (i > 0) *outStream << "," << sp;
                                writeJsonObject(filtered[i], *outStream);
                            }
                            *outStream << sp << "]";
                        }
                    }
                    
                    if (outputFile.empty()) {
                        outStream->flush();  // Flush for true streaming on stdout
                    }
                }
                
                if (!hasInput) {
                    std::cerr << "Error: No input data" << std::endl;
                    if (!outputFile.empty()) {
                        outFile.close();
                    }
                    return;
                }
                
                *outStream << "\n";
                if (!outputFile.empty()) {
                    outFile.close();
                    if (verbosity >= 1) {
                        std::cerr << "Wrote json to " << outputFile << std::endl;
                    }
                }
                return;
            }
            
            // Buffered path: needed for CSV output (must collect all keys first for header)
            std::vector<std::string> stdinLines;
            std::string line;
            std::set<std::string> stdinKeys;
            
            while (std::getline(std::cin, line)) {
                if (line.empty()) continue;
                stdinLines.push_back(line);
                
                if (inputFormat == "csv") {
                    // First line is header for CSV
                    if (stdinKeys.empty()) {
                        auto fields = CsvParser::parseCsvLine(line);
                        for (const auto& field : fields) {
                            if (!field.empty()) {
                                stdinKeys.insert(field);
                            }
                        }
                    }
                } else {
                    // JSON format
                    auto readings = JsonParser::parseJsonLine(line);
                    for (const auto& reading : readings) {
                        for (const auto& pair : reading) {
                            stdinKeys.insert(pair.first);
                        }
                    }
                }
            }
            
            if (stdinLines.empty()) {
                std::cerr << "Error: No input data" << std::endl;
                return;
            }
            
            allKeys = stdinKeys;
            std::vector<std::string> headers(allKeys.begin(), allKeys.end());
            std::sort(headers.begin(), headers.end());
            
            // Write to output
            if (outputFile.empty()) {
                // Write to stdout
                if (outputFormat == "json") {
                    // JSON output - line by line format matching .out files
                    processStdinDataJson(stdinLines, std::cout);
                    std::cout << "\n";
                } else {
                    // CSV output - write header first
                    for (size_t i = 0; i < headers.size(); ++i) {
                        if (i > 0) std::cout << ",";
                        std::cout << headers[i];
                    }
                    std::cout << "\n";
                    
                    // Process stdin data
                    processStdinData(stdinLines, headers, std::cout);
                }
            } else {
                std::ofstream outfile(outputFile);
                if (!outfile) {
                    std::cerr << "Error: Cannot create output file: " << outputFile << std::endl;
                    return;
                }
                
                if (outputFormat == "json") {
                    // JSON output - line by line format matching .out files
                    processStdinDataJson(stdinLines, outfile);
                    outfile << "\n";
                } else {
                    // Write CSV header
                    for (size_t i = 0; i < headers.size(); ++i) {
                        if (i > 0) outfile << ",";
                        outfile << headers[i];
                    }
                    outfile << "\n";
                    
                    // Process stdin data
                    processStdinData(stdinLines, headers, outfile);
                }
                
                outfile.close();
                if (verbosity >= 1) {
                    std::cerr << "Wrote " << outputFormat << " to " << outputFile << std::endl;
                }
            }
            return;
        }
        
        // Original file-based processing
        printCommonVerboseInfo("Starting conversion", verbosity, recursive, extensionFilter, maxDepth, inputFiles.size());
        
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
        }
        
        // PASS 1: Collect all column names (skip if using prototype)
        if (usePrototype) {
            std::cerr << "Using sc-prototype for column definitions..." << std::endl;
            if (!getPrototypeColumns()) {
                std::cerr << "Error: Failed to get prototype columns" << std::endl;
                return;
            }
        } else {
            std::cerr << "Pass 1: Discovering columns..." << std::endl;
            
            size_t filesPerThread = std::max(size_t(1), inputFiles.size() / numThreads);
            std::vector<std::future<void>> futures;
            futures.reserve(numThreads);
            
            for (size_t i = 0; i < inputFiles.size(); i += filesPerThread) {
                size_t end = std::min(i + filesPerThread, inputFiles.size());
                
                futures.push_back(std::async(std::launch::async, [this, i, end]() {
                    for (size_t j = i; j < end; ++j) {
                        collectKeysFromFile(inputFiles[j]);
                    }
                }));
            }
            
            // Wait for all threads to complete
            for (auto& f : futures) {
                f.wait();
            }
            
            std::cerr << "Found " << allKeys.size() << " unique fields" << std::endl;
        }
        
        // Convert set to vector for consistent ordering
        std::vector<std::string> headers(allKeys.begin(), allKeys.end());
        std::sort(headers.begin(), headers.end());
        
        // PASS 2: Stream data to output
        if (outputFile.empty()) {
            if (verbosity >= 1) {
                std::cerr << "Pass 2: Writing " << outputFormat << " to stdout..." << std::endl;
            }
            
            if (outputFormat == "json") {
                // JSON output - line by line format matching .out files
                bool firstOutput = true;
                for (const auto& file : inputFiles) {
                    writeRowsFromFileJson(file, std::cout, firstOutput);
                }
                std::cout << "\n";
            } else {
                // CSV output - write header first
                for (size_t i = 0; i < headers.size(); ++i) {
                    if (i > 0) std::cout << ",";
                    std::cout << headers[i];
                }
                std::cout << "\n";
                
                // Write rows to stdout
                for (const auto& file : inputFiles) {
                    writeRowsFromFile(file, std::cout, headers);
                }
            }
        } else {
            if (verbosity >= 1) {
                std::cerr << "Pass 2: Writing " << outputFormat << " to file..." << std::endl;
            }
            
            std::ofstream outfile(outputFile);
            if (!outfile) {
                std::cerr << "Error: Cannot create output file: " << outputFile << std::endl;
                return;
            }
            
            if (outputFormat == "json") {
                // JSON output - line by line format matching .out files
                bool firstOutput = true;
                for (const auto& file : inputFiles) {
                    writeRowsFromFileJson(file, outfile, firstOutput);
                }
                outfile << "\n";
            } else {
                // Write CSV header
                for (size_t i = 0; i < headers.size(); ++i) {
                    if (i > 0) outfile << ",";
                    outfile << headers[i];
                }
                outfile << "\n";
                
                // Write rows from each file sequentially (streaming)
                for (const auto& file : inputFiles) {
                    writeRowsFromFile(file, outfile, headers);
                }
            }
            
            outfile.close();
            if (verbosity >= 1) {
                std::cerr << "Wrote " << outputFormat << " to " << outputFile << std::endl;
            }
        }
    }
    
    static void printConvertUsage(const char* progName) {
        std::cerr << "Usage: " << progName << " convert [options] [<input_file(s)_or_directory(ies)>]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Convert JSON or CSV sensor data files to JSON or CSV format." << std::endl;
        std::cerr << "For JSON: Each line in input files should contain JSON with sensor readings." << std::endl;
        std::cerr << "For CSV: Files with .csv extension are automatically detected and processed." << std::endl;
        std::cerr << "Each sensor reading will become a row in the output." << std::endl;
        std::cerr << "If no input files are specified, reads from stdin (assumes JSON format unless -f is used)." << std::endl;
        std::cerr << "Output is written to stdout unless -o/--output is specified." << std::endl;
        std::cerr << "Default output format: JSON (matching .out file format)." << std::endl;
        std::cerr << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << "  -o, --output <file>       Output file (default: stdout)" << std::endl;
        std::cerr << "  -f, --format <fmt>        Input format for stdin: json or csv (default: json)" << std::endl;
        std::cerr << "  -F, --output-format <fmt> Output format: json or csv (default: json)" << std::endl;
        std::cerr << "  -r, --recursive           Recursively process subdirectories" << std::endl;
        std::cerr << "  -v                        Verbose output (show progress)" << std::endl;
        std::cerr << "  -V                        Very verbose output (show detailed progress)" << std::endl;
        std::cerr << "  -e, --extension <ext>     Filter files by extension (e.g., .out or out)" << std::endl;
        std::cerr << "  -d, --depth <n>           Maximum recursion depth (0 = current dir only)" << std::endl;
        std::cerr << "  --use-prototype           Use sc-prototype command to define columns" << std::endl;
        std::cerr << "  --not-empty <column>      Skip rows where column is empty (can be used multiple times)" << std::endl;
        std::cerr << "  --only-value <col:val>    Only include rows where column has specific value (can be used multiple times)" << std::endl;
        std::cerr << "  --exclude-value <col:val> Exclude rows where column has specific value (can be used multiple times)" << std::endl;
        std::cerr << "  --remove-errors           Remove error readings (DS18B20 value=85 or -127)" << std::endl;
        std::cerr << "  --remove-whitespace       Remove extra whitespace from output (compact format)" << std::endl;
        std::cerr << "  --min-date <date>         Filter readings after this date (Unix timestamp, ISO date, or DD/MM/YYYY)" << std::endl;
        std::cerr << "  --max-date <date>         Filter readings before this date (Unix timestamp, ISO date, or DD/MM/YYYY)" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Examples:" << std::endl;
        std::cerr << "  " << progName << " convert sensor1.out" << std::endl;
        std::cerr << "  " << progName << " convert < sensor1.out" << std::endl;
        std::cerr << "  " << progName << " convert -f csv < sensor1.csv" << std::endl;
        std::cerr << "  cat sensor1.out | " << progName << " convert" << std::endl;
        std::cerr << "  cat sensor1.out | " << progName << " convert -o output.csv" << std::endl;
        std::cerr << "  " << progName << " convert -o output.csv sensor1.out" << std::endl;
        std::cerr << "  " << progName << " convert -o output.csv sensor1.csv sensor2.csv" << std::endl;
        std::cerr << "  " << progName << " convert -o output.csv sensor1.out sensor2.out" << std::endl;
        std::cerr << "  " << progName << " convert --remove-errors -o output.csv sensor1.out" << std::endl;
        std::cerr << "  " << progName << " convert -e .out -o output.csv /path/to/sensor/dir" << std::endl;
        std::cerr << "  " << progName << " convert -r -e .csv -o output.csv /path/to/sensor/dir" << std::endl;
        std::cerr << "  " << progName << " convert -r -e .out -o output.csv /path/to/sensor/dir" << std::endl;
        std::cerr << "  " << progName << " convert -r -d 2 -e .out -o output.csv /path/to/logs" << std::endl;
        std::cerr << "  " << progName << " convert --use-prototype -r -e .out -o output.csv /path/to/logs" << std::endl;
        std::cerr << "  " << progName << " convert --not-empty unit --not-empty value -e .out -o output.csv /logs" << std::endl;
        std::cerr << "  " << progName << " convert --only-value type:temperature -r -e .out -o output.csv /logs" << std::endl;
        std::cerr << "  " << progName << " convert --only-value type:temperature --only-value unit:C -o output.csv /logs" << std::endl;
    }
};

#endif // SENSOR_DATA_CONVERTER_H
