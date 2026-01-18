#ifndef DATA_READER_H
#define DATA_READER_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "date_utils.h"
#include "csv_parser.h"
#include "json_parser.h"
#include "file_utils.h"

// Centralized data reader that handles file/stdin, CSV/JSON, and date filtering
class DataReader {
private:
    long long minDate;
    long long maxDate;
    int verbosity;
    std::string inputFormat;  // "json" or "csv"
    
    bool passesDateFilter(const std::map<std::string, std::string>& reading) const {
        if (minDate <= 0 && maxDate <= 0) return true;
        long long timestamp = DateUtils::getTimestamp(reading);
        return DateUtils::isInDateRange(timestamp, minDate, maxDate);
    }
    
public:
    DataReader(long long minDate = 0, long long maxDate = 0, int verbosity = 0, const std::string& format = "json")
        : minDate(minDate), maxDate(maxDate), verbosity(verbosity), inputFormat(format) {}
    
    // Internal helper to process a stream (CSV or JSON format)
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
                std::map<std::string, std::string> reading;
                for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                    reading[csvHeaders[i]] = fields[i];
                }
                
                if (!passesDateFilter(reading)) continue;
                
                callback(reading, lineNum, sourceName);
            }
        } else {
            // JSON format
            while (std::getline(input, line)) {
                lineNum++;
                if (line.empty()) continue;
                
                auto readings = JsonParser::parseJsonLine(line);
                for (const auto& reading : readings) {
                    if (reading.empty()) continue;
                    
                    if (!passesDateFilter(reading)) continue;
                    
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
        processStream(std::cin, inputFormat == "csv", callback, "stdin");
    }
    
    // Process readings from a file
    template<typename Callback>
    void processFile(const std::string& filename, Callback callback) {
        if (verbosity >= 1) {
            std::cout << "Processing file: " << filename << std::endl;
        }
        
        std::ifstream infile(filename);
        if (!infile) {
            std::cerr << "Warning: Cannot open file: " << filename << std::endl;
            return;
        }
        
        processStream(infile, FileUtils::isCsvFile(filename), callback, filename);
        infile.close();
    }
};

#endif // DATA_READER_H
