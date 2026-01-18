// Debug tool to compare line-by-line counting
// Compile: g++ -std=c++11 -Iinclude -o debug_count debug_count.cpp src/json_parser.cpp src/error_detector.cpp src/file_utils.cpp

#include <iostream>
#include <fstream>
#include <string>
#include "json_parser.h"
#include "error_detector.h"

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
    
    while (std::getline(infile, line)) {
        lineNum++;
        
        if (line.empty()) continue;
        
        auto readings = JsonParser::parseJsonLine(line);
        
        // Count WITHOUT --clean: just count non-empty readings
        long long countWithout = 0;
        for (const auto& reading : readings) {
            if (!reading.empty()) {
                countWithout++;
            }
        }
        
        // Count WITH --clean: skip empty JSON, check value non-empty, check errors
        long long countWith = 0;
        
        // Check if all readings are empty (removeEmptyJson logic)
        bool allEmpty = true;
        for (const auto& r : readings) {
            if (!r.empty()) { allEmpty = false; break; }
        }
        
        if (!allEmpty) {
            for (const auto& reading : readings) {
                if (reading.empty()) continue;
                
                // Check value is non-empty (--not-empty value)
                auto it = reading.find("value");
                if (it == reading.end() || it->second.empty()) continue;
                
                // Check for errors (--remove-errors)
                if (ErrorDetector::isErrorReading(reading)) continue;
                
                countWith++;
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
            std::cout << "  First 100 chars: " << line.substr(0, 100) << std::endl;
            
            // Check for null bytes
            for (size_t i = 0; i < line.length() && i < 20; i++) {
                if (line[i] == '\0') {
                    std::cout << "  NULL BYTE at position " << i << std::endl;
                }
            }
            std::cout << std::endl;
        }
    }
    
    std::cout << "Total lines: " << lineNum << std::endl;
    std::cout << "Total WITHOUT --clean: " << totalWithout << std::endl;
    std::cout << "Total WITH --clean: " << totalWith << std::endl;
    std::cout << "Difference: " << (totalWith - totalWithout) << std::endl;
    
    return 0;
}
