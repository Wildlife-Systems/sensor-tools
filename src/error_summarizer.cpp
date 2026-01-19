#include "error_summarizer.h"

#include "data_reader.h"
#include "error_detector.h"

// ===== Constructor =====

ErrorSummarizer::ErrorSummarizer(int argc, char* argv[]) {
    // Check for help flag first
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printSummariseErrorsUsage(argv[0]);
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
        printSummariseErrorsUsage(argv[0]);
        exit(1);
    }
    
    copyFromParser(parser);
}

// ===== Main summariseErrors method =====

void ErrorSummarizer::summariseErrors() {
    if (inputFiles.empty()) {
        DataReader reader(minDate, maxDate, verbosity, inputFormat);
        auto countError = [&](const Reading& reading, int /*lineNum*/, const std::string& /*source*/) {
            if (ErrorDetector::isErrorReading(reading)) {
                std::string errorDesc = ErrorDetector::getErrorDescription(reading);
                errorCounts[errorDesc]++;
            }
        };
        reader.processStdin(countError);
    } else {
        printCommonVerboseInfo("Summarising errors", verbosity, recursive, extensionFilter, maxDepth, inputFiles.size());
        
        // Process files in parallel
        auto processFile = [this](const std::string& file) -> std::map<std::string, int> {
            std::map<std::string, int> localCounts;
            DataReader reader(minDate, maxDate, verbosity, inputFormat);
            
            reader.processFile(file, [&](const Reading& reading, int, const std::string&) {
                if (ErrorDetector::isErrorReading(reading)) {
                    std::string errorDesc = ErrorDetector::getErrorDescription(reading);
                    localCounts[errorDesc]++;
                }
            });
            
            return localCounts;
        };
        
        // Combine function: sum counts
        auto combineCounts = [](std::map<std::string, int>& combined, const std::map<std::string, int>& local) {
            for (const auto& pair : local) {
                combined[pair.first] += pair.second;
            }
        };
        
        errorCounts = processFilesParallel(inputFiles, processFile, combineCounts, 
                                           std::map<std::string, int>());
    }
    
    // Print summary
    if (errorCounts.empty()) {
        std::cout << "No errors found" << std::endl;
    } else {
        std::cout << "Error Summary:" << std::endl;
        int totalErrors = 0;
        for (const auto& pair : errorCounts) {
            std::cout << "  " << pair.first << ": " << pair.second << " occurrence" << (pair.second > 1 ? "s" : "") << std::endl;
            totalErrors += pair.second;
        }
        std::cout << "Total errors: " << totalErrors << std::endl;
    }
}

// ===== Usage printing =====

void ErrorSummarizer::printSummariseErrorsUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " summarise-errors [options] [<input_file(s)_or_directory(ies)>]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Summarise error readings in sensor data files with counts." << std::endl;
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
    std::cerr << "  " << progName << " summarise-errors sensor1.out" << std::endl;
    std::cerr << "  " << progName << " summarise-errors < sensor1.out" << std::endl;
    std::cerr << "  " << progName << " summarise-errors -f csv < sensor1.csv" << std::endl;
    std::cerr << "  cat sensor1.out | " << progName << " summarise-errors" << std::endl;
    std::cerr << "  " << progName << " summarise-errors -r -e .out /path/to/logs/" << std::endl;
    std::cerr << "  " << progName << " summarise-errors sensor1.csv sensor2.out" << std::endl;
}
