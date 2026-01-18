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
    
    // Process readings from stdin
    template<typename Callback>
    void processStdin(Callback callback) {
        if (verbosity >= 1) {
            std::cerr << "Reading from stdin (format: " << inputFormat << ")..." << std::endl;
        }
        
        std::string line;
        int lineNum = 0;
        
        if (inputFormat == "csv") {
            // CSV format
            std::vector<std::string> csvHeaders;
            if (std::getline(std::cin, line) && !line.empty()) {
                lineNum++;
                bool needMore = false;
                csvHeaders = CsvParser::parseCsvLine(std::cin, line, needMore);
            }
            
            while (std::getline(std::cin, line)) {
                lineNum++;
                if (line.empty()) continue;
                
                bool needMore = false;
                auto fields = CsvParser::parseCsvLine(std::cin, line, needMore);
                if (fields.empty()) continue;
                
                // Create a map from CSV headers to values
                std::map<std::string, std::string> reading;
                for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                    reading[csvHeaders[i]] = fields[i];
                }
                
                if (!passesDateFilter(reading)) continue;
                
                callback(reading, lineNum, "stdin");
            }
        } else {
            // JSON format
            while (std::getline(std::cin, line)) {
                lineNum++;
                if (line.empty()) continue;
                
                auto readings = JsonParser::parseJsonLine(line);
                for (const auto& reading : readings) {
                    if (reading.empty()) continue;
                    
                    if (!passesDateFilter(reading)) continue;
                    
                    callback(reading, lineNum, "stdin");
                }
            }
        }
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
        
        std::string line;
        int lineNum = 0;
        
        // Check if this is a CSV file
        if (FileUtils::isCsvFile(filename)) {
            // CSV format
            std::vector<std::string> csvHeaders;
            if (std::getline(infile, line) && !line.empty()) {
                lineNum++;
                bool needMore = false;
                csvHeaders = CsvParser::parseCsvLine(infile, line, needMore);
            }
            
            while (std::getline(infile, line)) {
                lineNum++;
                if (line.empty()) continue;
                
                bool needMore = false;
                auto fields = CsvParser::parseCsvLine(infile, line, needMore);
                if (fields.empty()) continue;
                
                // Create a map from CSV headers to values
                std::map<std::string, std::string> reading;
                for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                    reading[csvHeaders[i]] = fields[i];
                }
                
                if (!passesDateFilter(reading)) continue;
                
                callback(reading, lineNum, filename);
            }
        } else {
            // JSON format
            while (std::getline(infile, line)) {
                lineNum++;
                if (line.empty()) continue;
                
                auto readings = JsonParser::parseJsonLine(line);
                for (const auto& reading : readings) {
                    if (reading.empty()) continue;
                    
                    if (!passesDateFilter(reading)) continue;
                    
                    callback(reading, lineNum, filename);
                }
            }
        }
        
        infile.close();
    }
};

#endif // DATA_READER_H
