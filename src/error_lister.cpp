#include "error_lister.h"

#include "data_reader.h"
#include "error_detector.h"
#include <sstream>

// ===== Static helper method =====

void ErrorLister::printErrorLine(const Reading& reading, 
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

// ===== Constructor =====

ErrorLister::ErrorLister(int argc, char* argv[]) {
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
    
    // Check for unknown options (no additional options for this command)
    std::string unknownOpt = CommonArgParser::checkUnknownOptions(argc, argv);
    if (!unknownOpt.empty()) {
        std::cerr << "Error: Unknown option '" << unknownOpt << "'" << std::endl;
        printListErrorsUsage(argv[0]);
        exit(1);
    }
    
    copyFromParser(parser);
}

// ===== Main listErrors method =====

void ErrorLister::listErrors() {
    if (inputFiles.empty()) {
        DataReader reader = createDataReader();
        reader.processStdin([](const Reading& reading, int lineNum, const std::string& source) {
            // Filtering already done by DataReader - just check if it's an error reading
            if (!ErrorDetector::isErrorReading(reading)) return;
            
            std::cout << source << ":" << lineNum;
            
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
        });
        return;
    }
    
    printCommonVerboseInfo("Listing errors", verbosity, recursive, extensionFilter, maxDepth, inputFiles.size());
    
    // Process files in parallel, collecting error output strings
    auto processFile = [this](const std::string& file) -> std::vector<std::string> {
        std::vector<std::string> errorLines;
        DataReader reader = createDataReader();
        
        reader.processFile(file, [&](const Reading& reading, 
                                      int lineNum, const std::string& source) {
            // Filtering already done by DataReader - just check if it's an error reading
            if (!ErrorDetector::isErrorReading(reading)) return;
            
            std::ostringstream oss;
            oss << source << ":" << lineNum;
            
            // Print relevant fields
            auto sensorIt = reading.find("sensor");
            auto valueIt = reading.find("value");
            auto tempIt = reading.find("temperature");
            auto idIt = reading.find("sensor_id");
            auto nameIt = reading.find("name");
            
            if (sensorIt != reading.end()) oss << " sensor=" << sensorIt->second;
            if (idIt != reading.end()) oss << " sensor_id=" << idIt->second;
            if (nameIt != reading.end()) oss << " name=" << nameIt->second;
            if (valueIt != reading.end()) oss << " value=" << valueIt->second;
            if (tempIt != reading.end()) oss << " temperature=" << tempIt->second;
            
            std::string errorDesc = ErrorDetector::getErrorDescription(reading);
            oss << " [" << errorDesc << "]";
            
            errorLines.push_back(oss.str());
        });
        
        return errorLines;
    };
    
    // Combine function: concatenate error lines
    auto combineErrors = [](std::vector<std::string>& combined, const std::vector<std::string>& local) {
        combined.insert(combined.end(), local.begin(), local.end());
    };
    
    std::vector<std::string> allErrors = processFilesParallel(inputFiles, processFile, combineErrors, 
                                                               std::vector<std::string>());
    
    // Output all collected errors
    for (const auto& line : allErrors) {
        std::cout << line << std::endl;
    }
}

// ===== Usage printing =====

void ErrorLister::printListErrorsUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " list-errors [options] [<input_file(s)_or_directory(ies)>]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "List error readings in sensor data files." << std::endl;
    std::cerr << "Currently detects DS18B20 sensors with temperature/value of 85 or -127 (error conditions)." << std::endl;
    std::cerr << "If no input files are specified, reads from stdin (assumes JSON format unless -if is used)." << std::endl;
    std::cerr << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  -if, --input-format <fmt> Input format for stdin: json or csv (default: json)" << std::endl;
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
