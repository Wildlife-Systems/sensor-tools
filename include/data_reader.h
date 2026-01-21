#ifndef DATA_READER_H
#define DATA_READER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <thread>
#include <chrono>
#include "types.h"
#include "csv_parser.h"
#include "json_parser.h"
#include "file_utils.h"
#include "reading_filter.h"

/**
 * DataReader - Centralized data reader with integrated filtering.
 * 
 * Handles:
 * - File and stdin input
 * - CSV and JSON format detection/parsing
 * - ALL filtering via ReadingFilter
 * - Tail mode for reading last N lines
 * - Collecting all readings for multi-pass processing
 * 
 * Flow: Input → Parse → Filter → Callback (or collection)
 * 
 * Commands receive ONLY readings that pass all filters.
 * For multi-pass scenarios (like collecting keys then writing),
 * use collectFromStdin() or collectFromFile() instead of manual parsing.
 */
class DataReader {
private:
    ReadingFilter ownedFilter;
    ReadingFilter* sharedFilter;  // if non-null, use this instead of ownedFilter
    int verbosity;
    std::string inputFormat;  // "json", "csv", or "auto"
    int tailLines;  // 0 = read all, >0 = read only last n lines
    
    // --tail-column-value column:value n: return last n rows where column=value
    std::string tailColumnValueColumn;
    std::string tailColumnValueValue;
    int tailColumnValueCount;
    
    // Get the active filter (shared or owned)
    ReadingFilter& filter() {
        return sharedFilter ? *sharedFilter : ownedFilter;
    }
    const ReadingFilter& filter() const {
        return sharedFilter ? *sharedFilter : ownedFilter;
    }
    
public:
    DataReader(int verbosity = 0, const std::string& format = "auto", int tailLines = 0)
        : sharedFilter(nullptr), verbosity(verbosity), inputFormat(format), tailLines(tailLines), tailColumnValueCount(0) {
        ownedFilter.setVerbosity(verbosity);
    }
    
    // Constructor that uses a shared filter (for thread-safe --unique across files)
    DataReader(ReadingFilter& shared, int verbosity = 0, const std::string& format = "auto", int tailLines = 0)
        : sharedFilter(&shared), verbosity(verbosity), inputFormat(format), tailLines(tailLines), tailColumnValueCount(0) {
    }
    
    // Set tail-column-value filter (reads file backwards for efficiency)
    void setTailColumnValue(const std::string& column, const std::string& value, int count) {
        tailColumnValueColumn = column;
        tailColumnValueValue = value;
        tailColumnValueCount = count;
    }
    
    // Get mutable reference to filter for configuration
    ReadingFilter& getFilter() {
        return filter();
    }
    
    // Convenience setters that delegate to filter
    void setDateRange(long long minDate, long long maxDate) {
        filter().setDateRange(minDate, maxDate);
    }
    
    void setRemoveErrors(bool remove) {
        filter().setRemoveErrors(remove);
    }
    
    void setVerbosity(int v) {
        verbosity = v;
        filter().setVerbosity(v);
    }
    
    // Internal helper to process a stream (CSV or JSON format)
    // Filtering is applied here - callbacks only receive filtered readings
    template<typename Callback>
    void processStream(std::istream& input, bool isCSV, Callback callback, const std::string& sourceName) {
        std::string line;
        int lineNum = 0;
        
        if (isCSV) {
            // CSV format - first line is header
            std::vector<std::string> csvHeaders;
            if (std::getline(input, line) && !line.empty()) {
                lineNum++;
                bool needMore = false;
                csvHeaders = CsvParser::parseCsvLine(input, line, needMore);
            }
            
            while (std::getline(input, line)) {
                lineNum++;
                if (line.empty()) continue;
                
                bool needMore = false;
                auto fields = CsvParser::parseCsvLine(input, line, needMore);
                if (fields.empty()) continue;
                
                // Create a map from CSV headers to values
                Reading reading;
                reading.reserve(csvHeaders.size());
                for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                    reading.emplace(csvHeaders[i], std::move(fields[i]));
                }
                
                // Apply ALL filters here
                if (!filter().shouldInclude(reading)) continue;
                
                // Apply transformations (updates) after filtering
                filter().applyTransformations(reading);
                
                callback(reading, lineNum, sourceName);
            }
        } else {
            // JSON format
            while (std::getline(input, line)) {
                lineNum++;
                if (line.empty()) continue;
                
                auto readings = JsonParser::parseJsonLine(line);
                for (auto reading : readings) {  // Copy to allow mutation
                    if (reading.empty()) continue;
                    
                    // Apply ALL filters here
                    if (!filter().shouldInclude(reading)) continue;
                    
                    // Apply transformations (updates) after filtering
                    filter().applyTransformations(reading);
                    
                    callback(reading, lineNum, sourceName);
                }
            }
        }
    }
    
    // Process readings from stdin
    template<typename Callback>
    void processStdin(Callback callback) {
        if (verbosity >= 1) {
            std::cerr << "Reading from stdin (format: " << inputFormat << ")..." << std::endl;
        }
        // For stdin: "csv" means CSV, anything else (including "auto" and "json") means JSON
        processStream(std::cin, inputFormat == "csv", callback, "stdin");
    }
    
    // Process readings from a file
    template<typename Callback>
    void processFile(const std::string& filename, Callback callback) {
        if (verbosity >= 1) {
            std::cout << "Processing file: " << filename << std::endl;
            if (tailLines > 0) {
                std::cout << "  (reading last " << tailLines << " lines only)" << std::endl;
            }
        }
        
        // Determine format: explicit "csv"/"json" overrides, "auto" detects from extension
        bool isCSV;
        if (inputFormat == "csv") {
            isCSV = true;
        } else if (inputFormat == "json") {
            isCSV = false;
        } else {
            // "auto" or any other value: detect from file extension
            isCSV = FileUtils::isCsvFile(filename);
        }
        
        // Handle --tail-column-value: read backwards to find last n matching rows
        if (tailColumnValueCount > 0) {
            if (verbosity >= 1) {
                std::cerr << "  (finding last " << tailColumnValueCount << " rows where " 
                         << tailColumnValueColumn << "=" << tailColumnValueValue << ")" << std::endl;
            }
            
            std::vector<std::string> matchingLines;
            matchingLines.reserve(tailColumnValueCount);
            
            // For CSV, we need the header first
            std::vector<std::string> csvHeaders;
            std::string headerLine;
            if (isCSV) {
                std::ifstream headerFile(filename);
                if (!headerFile) {
                    std::cerr << "Warning: Cannot open file: " << filename << std::endl;
                    return;
                }
                if (std::getline(headerFile, headerLine) && !headerLine.empty()) {
                    bool needMore = false;
                    csvHeaders = CsvParser::parseCsvLine(headerFile, headerLine, needMore);
                }
                headerFile.close();
            }
            
            // Read file backwards, collecting matching lines
            FileUtils::readLinesReverse(filename, [&](const std::string& line) -> bool {
                if (line.empty()) return true;  // continue
                if (isCSV && line == headerLine) return true;  // skip header
                
                // Parse and check for match
                if (isCSV) {
                    auto fields = CsvParser::parseCsvLine(line);
                    if (fields.empty()) return true;
                    
                    Reading reading;
                    for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                        reading[csvHeaders[i]] = fields[i];
                    }
                    
                    // Check column match
                    auto it = reading.find(tailColumnValueColumn);
                    if (it != reading.end() && it->second == tailColumnValueValue) {
                        // Check other filters
                        if (filter().shouldInclude(reading)) {
                            matchingLines.push_back(line);
                            if (static_cast<int>(matchingLines.size()) >= tailColumnValueCount) {
                                return false;  // stop reading
                            }
                        }
                    }
                } else {
                    // JSON
                    auto readings = JsonParser::parseJsonLine(line);
                    for (const auto& reading : readings) {
                        if (reading.empty()) continue;
                        
                        auto it = reading.find(tailColumnValueColumn);
                        if (it != reading.end() && it->second == tailColumnValueValue) {
                            if (filter().shouldInclude(reading)) {
                                matchingLines.push_back(line);
                                if (static_cast<int>(matchingLines.size()) >= tailColumnValueCount) {
                                    return false;  // stop reading
                                }
                            }
                        }
                    }
                }
                return true;  // continue reading
            });
            
            // Reverse to get chronological order, then process
            std::reverse(matchingLines.begin(), matchingLines.end());
            int lineNum = 0;
            for (const auto& line : matchingLines) {
                lineNum++;
                if (isCSV) {
                    auto fields = CsvParser::parseCsvLine(line);
                    Reading reading;
                    for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                        reading[csvHeaders[i]] = fields[i];
                    }
                    filter().applyTransformations(reading);
                    callback(reading, lineNum, filename);
                } else {
                    auto readings = JsonParser::parseJsonLine(line);
                    for (auto reading : readings) {
                        filter().applyTransformations(reading);
                        callback(reading, lineNum, filename);
                    }
                }
            }
            return;  // Done with tail-column-value processing
        }
        
        if (tailLines > 0) {
            // Use tail mode
            if (isCSV) {
                // CSV: read header from file, then process tail lines
                std::ifstream headerFile(filename);
                if (!headerFile) {
                    std::cerr << "Warning: Cannot open file: " << filename << std::endl;
                    return;
                }
                std::string headerLine;
                std::vector<std::string> csvHeaders;
                if (std::getline(headerFile, headerLine) && !headerLine.empty()) {
                    bool needMore = false;
                    csvHeaders = CsvParser::parseCsvLine(headerFile, headerLine, needMore);
                }
                headerFile.close();
                
                // Read tail lines - we want exactly tailLines data rows
                // If the header happens to be in the tail (small file), we'll skip it
                auto lines = FileUtils::readTailLines(filename, tailLines);
                int lineNum = 0;
                
                for (auto& line : lines) {
                    lineNum++;
                    if (line.empty()) continue;
                    // Skip if this is the header line (can happen with small files)
                    if (line == headerLine) continue;
                    
                    auto fields = CsvParser::parseCsvLine(line);
                    if (fields.empty()) continue;
                    
                    Reading reading;
                    reading.reserve(csvHeaders.size());
                    for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                        reading.emplace(csvHeaders[i], std::move(fields[i]));
                    }
                    
                    // Apply ALL filters here
                    if (!filter().shouldInclude(reading)) continue;
                    
                    // Apply transformations (updates) after filtering
                    filter().applyTransformations(reading);
                    
                    callback(reading, lineNum, filename);
                }
            } else {
                // JSON: just process tail lines
                auto lines = FileUtils::readTailLines(filename, tailLines);
                int lineNum = 0;
                
                for (const auto& line : lines) {
                    lineNum++;
                    if (line.empty()) continue;
                    
                    auto readings = JsonParser::parseJsonLine(line);
                    for (auto reading : readings) {  // Copy to allow mutation
                        if (reading.empty()) continue;
                        
                        // Apply ALL filters here
                        if (!filter().shouldInclude(reading)) continue;
                        
                        // Apply transformations (updates) after filtering
                        filter().applyTransformations(reading);
                        
                        callback(reading, lineNum, filename);
                    }
                }
            }
        } else {
            // Use larger buffer for better I/O performance
            static constexpr size_t BUFFER_SIZE = 256 * 1024;  // 256KB buffer
            static thread_local char buffer[BUFFER_SIZE];
            
            std::ifstream infile;
            infile.rdbuf()->pubsetbuf(buffer, BUFFER_SIZE);
            infile.open(filename);
            
            if (!infile) {
                std::cerr << "ERROR: Failed to open file: " << filename << std::endl;
                return;
            }
            
            processStream(infile, isCSV, callback, filename);
            infile.close();
        }
    }
    
    // ===== Collection methods for multi-pass processing =====
    // These return all filtered readings at once, useful when you need
    // to iterate over the data multiple times (e.g., collect keys then write)
    
    /**
     * Collect all filtered readings from stdin.
     * Use this when you need to process stdin data in multiple passes.
     * The DataReader handles all parsing and filtering.
     */
    ReadingList collectFromStdin() {
        ReadingList results;
        processStdin([&](const Reading& reading, int /*lineNum*/, const std::string& /*source*/) {
            results.push_back(reading);
        });
        return results;
    }
    
    /**
     * Collect all filtered readings from a file.
     * Use this when you need to process file data in multiple passes.
     * The DataReader handles all parsing and filtering.
     */
    ReadingList collectFromFile(const std::string& filename) {
        ReadingList results;
        processFile(filename, [&](const Reading& reading, int /*lineNum*/, const std::string& /*source*/) {
            results.push_back(reading);
        });
        return results;
    }
    
    /**
     * Collect all filtered readings from multiple files.
     * Use this when you need to process multiple files in multiple passes.
     */
    ReadingList collectFromFiles(const std::vector<std::string>& files) {
        ReadingList results;
        for (const auto& file : files) {
            processFile(file, [&](const Reading& reading, int /*lineNum*/, const std::string& /*source*/) {
                results.push_back(reading);
            });
        }
        return results;
    }
    
    // ===== Follow mode methods =====
    // These continuously read input and call the callback for each filtered reading.
    // Uses the same filter.shouldInclude() as all other methods.
    
    /**
     * Continuously read from stdin and process each filtered reading.
     * Blocks forever, calling callback for each new reading that passes the filter.
     */
    template<typename Callback>
    void processStdinFollow(Callback callback) {
        if (verbosity >= 1) {
            std::cerr << "Reading from stdin with follow mode..." << std::endl;
        }
        
        std::string line;
        std::vector<std::string> csvHeaders;
        bool headerParsed = false;
        bool isCSV = (inputFormat == "csv");
        int lineNum = 0;
        
        while (true) {
            if (std::getline(std::cin, line)) {
                lineNum++;
                if (line.empty()) continue;
                
                if (isCSV) {
                    if (!headerParsed) {
                        bool needMore = false;
                        csvHeaders = CsvParser::parseCsvLine(std::cin, line, needMore);
                        headerParsed = true;
                        continue;
                    }
                    
                    bool needMore = false;
                    auto fields = CsvParser::parseCsvLine(std::cin, line, needMore);
                    if (fields.empty()) continue;
                    
                    Reading reading;
                    for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                        reading[csvHeaders[i]] = fields[i];
                    }
                    
                    // Use the SAME filter as all other methods
                    if (!filter().shouldInclude(reading)) continue;
                    
                    // Apply transformations (updates) after filtering
                    filter().applyTransformations(reading);
                    
                    callback(reading, lineNum, "stdin");
                } else {
                    auto readings = JsonParser::parseJsonLine(line);
                    for (auto reading : readings) {  // Copy to allow mutation
                        if (reading.empty()) continue;
                        
                        // Use the SAME filter as all other methods
                        if (!filter().shouldInclude(reading)) continue;
                        
                        // Apply transformations (updates) after filtering
                        filter().applyTransformations(reading);
                        
                        callback(reading, lineNum, "stdin");
                    }
                }
            } else {
                std::cin.clear();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
    
    /**
     * Follow a file and process each filtered reading (like tail -f).
     * First processes existing content, then waits for new content.
     * Calls callback for each reading that passes the filter.
     */
    template<typename Callback>
    void processFileFollow(const std::string& filename, Callback callback) {
        if (verbosity >= 1) {
            std::cerr << "Following file: " << filename << std::endl;
        }
        
        std::ifstream infile(filename);
        if (!infile) {
            std::cerr << "Error: Cannot open file: " << filename << std::endl;
            return;
        }
        
        std::string line;
        std::vector<std::string> csvHeaders;
        bool isCSV = FileUtils::isCsvFile(filename);
        int lineNum = 0;
        
        // Read CSV header if needed
        if (isCSV) {
            if (std::getline(infile, line) && !line.empty()) {
                lineNum++;
                bool needMore = false;
                csvHeaders = CsvParser::parseCsvLine(infile, line, needMore);
            }
        }
        
        // Process existing content and follow for new content
        while (true) {
            if (std::getline(infile, line)) {
                lineNum++;
                if (line.empty()) continue;
                
                if (isCSV) {
                    bool needMore = false;
                    auto fields = CsvParser::parseCsvLine(infile, line, needMore);
                    if (fields.empty()) continue;
                    
                    Reading reading;
                    for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                        reading[csvHeaders[i]] = fields[i];
                    }
                    
                    // Use the SAME filter as all other methods
                    if (!filter().shouldInclude(reading)) continue;
                    
                    // Apply transformations (updates) after filtering
                    filter().applyTransformations(reading);
                    
                    callback(reading, lineNum, filename);
                } else {
                    auto readings = JsonParser::parseJsonLine(line);
                    for (auto reading : readings) {  // Copy to allow mutation
                        if (reading.empty()) continue;
                        
                        // Use the SAME filter as all other methods
                        if (!filter().shouldInclude(reading)) continue;
                        
                        // Apply transformations (updates) after filtering
                        filter().applyTransformations(reading);
                        
                        callback(reading, lineNum, filename);
                    }
                }
            } else {
                infile.clear();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
};

#endif // DATA_READER_H
