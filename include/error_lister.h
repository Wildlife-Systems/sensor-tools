#ifndef ERROR_LISTER_H
#define ERROR_LISTER_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>

#include "common_arg_parser.h"
#include "data_reader.h"
#include "error_detector.h"

class ErrorLister {
private:
    std::vector<std::string> inputFiles;
    bool recursive;
    std::string extensionFilter;
    int maxDepth;
    int verbosity;
    std::string inputFormat;
    long long minDate;
    long long maxDate;

public:
    ErrorLister(int argc, char* argv[]) {
        // Check for help flag first
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--help" || arg == "-h") {
                printListErrorsUsage(argv[0]);
                exit(0);
            }
        }
        
        CommonArgParser parser;
        if (!parser.parse(argc, argv)) {
            exit(1);
        }
        
        // Check for unknown options
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg[0] == '-' && arg != "-r" && arg != "--recursive" && 
                arg != "-v" && arg != "-V" && arg != "-f" && arg != "--format" &&
                arg != "-e" && arg != "--extension" && arg != "-d" && arg != "--depth" &&
                arg != "--min-date" && arg != "--max-date" &&
                arg != "-h" && arg != "--help") {
                // Check if previous arg was a flag that takes an argument
                if (i > 1) {
                    std::string prev = argv[i-1];
                    if (prev == "-f" || prev == "--format" || prev == "-e" || prev == "--extension" ||
                        prev == "-d" || prev == "--depth" || prev == "--min-date" || prev == "--max-date") {
                        continue; // This is an argument value, not a flag
                    }
                }
                std::cerr << "Error: Unknown option '" << arg << "'" << std::endl;
                printListErrorsUsage(argv[0]);
                exit(1);
            }
        }
        
        // Copy parsed values
        recursive = parser.getRecursive();
        extensionFilter = parser.getExtensionFilter();
        maxDepth = parser.getMaxDepth();
        verbosity = parser.getVerbosity();
        inputFormat = parser.getInputFormat();
        minDate = parser.getMinDate();
        maxDate = parser.getMaxDate();
        inputFiles = parser.getInputFiles();
    }
    
    // Helper to print a single error line
    static void printErrorLine(const std::map<std::string, std::string>& reading, 
                               int lineNum, const std::string& source) {
        if (!ErrorDetector::isErrorReading(reading)) return;
        
        std::cout << source << ":" << lineNum;
        
        // Print relevant fields
        auto sensorIt = reading.find("sensor");
        auto valueIt = reading.find("value");
        auto tempIt = reading.find("temperature");
        auto idIt = reading.find("sensor_id");
        auto nameIt = reading.find("name");
        
        if (sensorIt != reading.end()) std::cout << " sensor=" << sensorIt->second;
        if (idIt != reading.end()) std::cout << " sensor_id=" << idIt->second;
        if (nameIt != reading.end()) std::cout << " name=" << nameIt->second;
        if (valueIt != reading.end()) std::cout << " value=" << valueIt->second;
        if (tempIt != reading.end()) std::cout << " temperature=" << tempIt->second;
        
        std::string errorDesc = ErrorDetector::getErrorDescription(reading);
        std::cout << " [" << errorDesc << "]" << std::endl;
    }
    
    void listErrors() {
        DataReader reader(minDate, maxDate, verbosity, inputFormat);
        
        if (inputFiles.empty()) {
            // Reading from stdin
            reader.processStdin(printErrorLine);
            return;
        }
        
        printCommonVerboseInfo("Listing errors", verbosity, recursive, extensionFilter, maxDepth, inputFiles.size());
        
        for (const auto& file : inputFiles) {
            reader.processFile(file, printErrorLine);
        }
    }
    
    static void printListErrorsUsage(const char* progName) {
        std::cerr << "Usage: " << progName << " list-errors [options] [<input_file(s)_or_directory(ies)>]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "List error readings in sensor data files." << std::endl;
        std::cerr << "Currently detects DS18B20 sensors with temperature/value of 85 or -127 (error conditions)." << std::endl;
        std::cerr << "If no input files are specified, reads from stdin (assumes JSON format unless -f is used)." << std::endl;
        std::cerr << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << "  -f, --format <fmt>        Input format for stdin: json or csv (default: json)" << std::endl;
        std::cerr << "  -r, --recursive           Recursively process subdirectories" << std::endl;
        std::cerr << "  -v                        Verbose output" << std::endl;
        std::cerr << "  -V                        Very verbose output" << std::endl;
        std::cerr << "  -e, --extension <ext>     Filter files by extension (e.g., .out or out)" << std::endl;
        std::cerr << "  -d, --depth <n>           Maximum recursion depth (0 = current dir only)" << std::endl;
        std::cerr << "  --min-date <date>         Filter readings after this date (Unix timestamp, ISO date, or DD/MM/YYYY)" << std::endl;
        std::cerr << "  --max-date <date>         Filter readings before this date (Unix timestamp, ISO date, or DD/MM/YYYY)" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Examples:" << std::endl;
        std::cerr << "  " << progName << " list-errors sensor1.out" << std::endl;
        std::cerr << "  " << progName << " list-errors < sensor1.out" << std::endl;
        std::cerr << "  " << progName << " list-errors -f csv < sensor1.csv" << std::endl;
        std::cerr << "  cat sensor1.out | " << progName << " list-errors" << std::endl;
        std::cerr << "  " << progName << " list-errors -r -e .out /path/to/logs/" << std::endl;
        std::cerr << "  " << progName << " list-errors sensor1.csv sensor2.out" << std::endl;
    }
};

#endif // ERROR_LISTER_H
