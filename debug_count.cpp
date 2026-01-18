// Debug tool to compare line-by-line counting
// Compile: g++ -std=c++11 -Iinclude -o debug_count debug_count.cpp src/json_parser.cpp src/error_detector.cpp src/file_utils.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include "json_parser.h"
#include "error_detector.h"

// Exact copy of shouldIncludeReading logic
bool shouldInclude(const std::map<std::string, std::string>& reading,
                   const std::set<std::string>& notEmptyColumns,
                   bool removeErrors) {
    // Check if any required columns are empty
    for (const auto& reqCol : notEmptyColumns) {
        auto it = reading.find(reqCol);
        if (it == reading.end() || it->second.empty()) {
            return false;
        }
    }
    
    // Check for error readings
    if (removeErrors && ErrorDetector::isErrorReading(reading)) {
        return false;
    }
    
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
        return 1;
    }
    
    std::ifstream infile(argv[1]);
    if (!infile) {
        std::cerr << "Cannot open file" << std::endl;
        return 1;
    }
    
    std::string line;
    long long lineNum = 0;
    long long totalWithout = 0;
    long long totalWith = 0;
    
    // Settings for --clean
    std::set<std::string> notEmptyColumnsClean;
    notEmptyColumnsClean.insert("value");
    std::set<std::string> notEmptyColumnsNone;
    
    while (std::getline(infile, line)) {
        lineNum++;
        
        if (line.empty()) continue;
        
        auto readings = JsonParser::parseJsonLine(line);
        
        // Count WITHOUT --clean (no filters)
        long long countWithout = 0;
        for (const auto& reading : readings) {
            if (reading.empty()) continue;
            if (shouldInclude(reading, notEmptyColumnsNone, false)) {
                countWithout++;
            }
        }
        
        // Count WITH --clean (removeEmptyJson + notEmpty value + removeErrors)
        long long countWith = 0;
        
        // removeEmptyJson check
        bool allEmpty = true;
        for (const auto& r : readings) {
            if (!r.empty()) { allEmpty = false; break; }
        }
        
        if (!allEmpty) {
            for (const auto& reading : readings) {
                if (reading.empty()) continue;
                if (shouldInclude(reading, notEmptyColumnsClean, true)) {
                    countWith++;
                }
            }
        }
        
        totalWithout += countWithout;
        totalWith += countWith;
        
        // Print if WITH gives more than WITHOUT (the bug!)
        if (countWith > countWithout) {
            std::cout << "LINE " << lineNum << ": without=" << countWithout 
                      << ", with=" << countWith << std::endl;
            std::cout << "  Line length: " << line.length() << std::endl;
            std::cout << "  Readings parsed: " << readings.size() << std::endl;
            if (line.length() > 100) {
                std::cout << "  First 100 chars: " << line.substr(0, 100) << std::endl;
            } else {
                std::cout << "  Content: " << line << std::endl;
            }
            std::cout << std::endl;
        }
    }
    
    std::cout << "Total lines read: " << lineNum << std::endl;
    std::cout << "Total WITHOUT --clean: " << totalWithout << std::endl;
    std::cout << "Total WITH --clean: " << totalWith << std::endl;
    std::cout << "Difference: " << (totalWith - totalWithout) << std::endl;
    
    return 0;
}
