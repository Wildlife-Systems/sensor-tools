#include "data_counter.h"
#include <fstream>
#include <algorithm>
#include <chrono>
#include <thread>

#include "csv_parser.h"
#include "json_parser.h"
#include "file_utils.h"
#include "file_collector.h"

// ===== Private methods =====

long long DataCounter::countFromFile(const std::string& filename) {
    if (verbosity >= 1) {
        std::cerr << "Counting: " << filename << std::endl;
    }

    std::ifstream infile(filename);
    if (!infile) {
        std::cerr << "Warning: Cannot open file: " << filename << std::endl;
        return 0;
    }

    long long count = 0;
    std::string line;

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

            std::map<std::string, std::string> reading;
            for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                reading[csvHeaders[i]] = fields[i];
            }

            if (shouldIncludeReading(reading)) {
                count++;
            }
        }
    } else {
        // JSON format
        while (std::getline(infile, line)) {
            if (line.empty()) continue;

            auto readings = JsonParser::parseJsonLine(line);
            
            // Skip empty JSON arrays/objects if removeEmptyJson is set
            if (removeEmptyJson) {
                bool allEmpty = true;
                for (const auto& r : readings) {
                    if (!r.empty()) { allEmpty = false; break; }
                }
                if (allEmpty) continue;
            }

            for (const auto& reading : readings) {
                if (reading.empty()) continue;
                if (shouldIncludeReading(reading)) {
                    count++;
                }
            }
        }
    }

    infile.close();
    return count;
}

long long DataCounter::countFromStdin() {
    if (verbosity >= 1) {
        std::cerr << "Reading from stdin..." << std::endl;
    }

    long long count = 0;
    std::string line;

    if (inputFormat == "csv") {
        // CSV format - first line is header
        std::vector<std::string> csvHeaders;
        if (std::getline(std::cin, line) && !line.empty()) {
            csvHeaders = CsvParser::parseCsvLine(line);
        }

        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;

            auto fields = CsvParser::parseCsvLine(line);
            if (fields.empty()) continue;

            std::map<std::string, std::string> reading;
            for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                reading[csvHeaders[i]] = fields[i];
            }

            if (shouldIncludeReading(reading)) {
                count++;
            }
        }
    } else {
        // JSON format
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;

            auto readings = JsonParser::parseJsonLine(line);
            
            if (removeEmptyJson) {
                bool allEmpty = true;
                for (const auto& r : readings) {
                    if (!r.empty()) { allEmpty = false; break; }
                }
                if (allEmpty) continue;
            }

            for (const auto& reading : readings) {
                if (reading.empty()) continue;
                if (shouldIncludeReading(reading)) {
                    count++;
                }
            }
        }
    }

    return count;
}

void DataCounter::countFromStdinFollow() {
    if (verbosity >= 1) {
        std::cerr << "Reading from stdin with follow mode..." << std::endl;
    }

    long long count = 0;
    std::string line;
    std::vector<std::string> csvHeaders;
    bool headerParsed = false;

    // Print initial count
    std::cout << count << std::endl;

    while (true) {
        if (std::getline(std::cin, line)) {
            if (line.empty()) continue;

            if (inputFormat == "csv") {
                if (!headerParsed) {
                    csvHeaders = CsvParser::parseCsvLine(line);
                    headerParsed = true;
                    continue;
                }

                auto fields = CsvParser::parseCsvLine(line);
                if (fields.empty()) continue;

                std::map<std::string, std::string> reading;
                for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                    reading[csvHeaders[i]] = fields[i];
                }

                if (shouldIncludeReading(reading)) {
                    count++;
                    std::cout << count << std::endl;
                }
            } else {
                // JSON format
                auto readings = JsonParser::parseJsonLine(line);
                
                if (removeEmptyJson) {
                    bool allEmpty = true;
                    for (const auto& r : readings) {
                        if (!r.empty()) { allEmpty = false; break; }
                    }
                    if (allEmpty) continue;
                }

                for (const auto& reading : readings) {
                    if (reading.empty()) continue;
                    if (shouldIncludeReading(reading)) {
                        count++;
                        std::cout << count << std::endl;
                    }
                }
            }
        } else {
            // No more data available, wait briefly and try again
            std::cin.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void DataCounter::countFromFileFollow(const std::string& filename) {
    if (verbosity >= 1) {
        std::cerr << "Following file: " << filename << std::endl;
    }

    std::ifstream infile(filename);
    if (!infile) {
        std::cerr << "Error: Cannot open file: " << filename << std::endl;
        return;
    }

    long long count = 0;
    std::string line;
    std::vector<std::string> csvHeaders;
    bool isCSV = FileUtils::isCsvFile(filename);

    // Read existing content first
    if (isCSV) {
        if (std::getline(infile, line) && !line.empty()) {
            bool needMore = false;
            csvHeaders = CsvParser::parseCsvLine(infile, line, needMore);
        }
    }

    while (std::getline(infile, line)) {
        if (line.empty()) continue;

        if (isCSV) {
            bool needMore = false;
            auto fields = CsvParser::parseCsvLine(infile, line, needMore);
            if (fields.empty()) continue;

            std::map<std::string, std::string> reading;
            for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                reading[csvHeaders[i]] = fields[i];
            }

            if (shouldIncludeReading(reading)) {
                count++;
            }
        } else {
            auto readings = JsonParser::parseJsonLine(line);
            
            if (removeEmptyJson) {
                bool allEmpty = true;
                for (const auto& r : readings) {
                    if (!r.empty()) { allEmpty = false; break; }
                }
                if (allEmpty) continue;
            }

            for (const auto& reading : readings) {
                if (reading.empty()) continue;
                if (shouldIncludeReading(reading)) {
                    count++;
                }
            }
        }
    }

    // Print initial count
    std::cout << count << std::endl;

    // Now follow the file for new content
    infile.clear();  // Clear EOF flag
    
    while (true) {
        if (std::getline(infile, line)) {
            if (line.empty()) continue;

            if (isCSV) {
                bool needMore = false;
                auto fields = CsvParser::parseCsvLine(infile, line, needMore);
                if (fields.empty()) continue;

                std::map<std::string, std::string> reading;
                for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                    reading[csvHeaders[i]] = fields[i];
                }

                if (shouldIncludeReading(reading)) {
                    count++;
                    std::cout << count << std::endl;
                }
            } else {
                auto readings = JsonParser::parseJsonLine(line);
                
                if (removeEmptyJson) {
                    bool allEmpty = true;
                    for (const auto& r : readings) {
                        if (!r.empty()) { allEmpty = false; break; }
                    }
                    if (allEmpty) continue;
                }

                for (const auto& reading : readings) {
                    if (reading.empty()) continue;
                    if (shouldIncludeReading(reading)) {
                        count++;
                        std::cout << count << std::endl;
                    }
                }
            }
        } else {
            // No more data available, wait briefly and try again
            infile.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

// ===== Constructor =====

DataCounter::DataCounter(int argc, char* argv[]) : followMode(false) {
    // Check for help flag first
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printCountUsage(argv[0]);
            exit(0);
        }
    }

    // Parse filter options
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        int result = parseFilterOption(argc, argv, i, arg);
        if (result >= 0) {
            i = result;
        }
    }
    
    // Parse --follow flag
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--follow" || arg == "-f") {
            followMode = true;
        }
    }

    // Parse common flags and collect files
    CommonArgParser parser;
    if (!parser.parse(argc, argv)) {
        exit(1);
    }

    copyFromParser(parser);

    // Check for unknown options (count-specific: -f/--follow)
    std::string unknownOpt = CommonArgParser::checkUnknownOptions(argc, argv, 
        {"-f", "--follow"});
    if (!unknownOpt.empty()) {
        std::cerr << "Error: Unknown option '" << unknownOpt << "'" << std::endl;
        printCountUsage(argv[0]);
        exit(1);
    }
}

// ===== Main count method =====

void DataCounter::count() {
    // Debug: Print filter state
    if (verbosity >= 1) {
        std::cerr << "removeErrors: " << (removeErrors ? "true" : "false") << std::endl;
        std::cerr << "removeEmptyJson: " << (removeEmptyJson ? "true" : "false") << std::endl;
        std::cerr << "notEmptyColumns: ";
        for (const auto& col : notEmptyColumns) {
            std::cerr << col << " ";
        }
        std::cerr << std::endl;
        printFilterInfo();
    }
    
    if (!hasInputFiles && followMode) {
        // Follow mode with stdin
        countFromStdinFollow();
        return;
    }
    
    if (followMode && inputFiles.size() == 1) {
        // Follow mode with a single file
        countFromFileFollow(inputFiles[0]);
        return;
    }
    
    if (followMode && inputFiles.size() > 1) {
        std::cerr << "Warning: --follow only supports a single file, using first file only" << std::endl;
        countFromFileFollow(inputFiles[0]);
        return;
    }
    
    long long totalCount = 0;

    if (!hasInputFiles) {
        totalCount = countFromStdin();
    } else {
        for (const auto& file : inputFiles) {
            totalCount += countFromFile(file);
        }
    }

    std::cout << totalCount << std::endl;
}

// ===== Usage printing =====

void DataCounter::printCountUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " count [options] [files/directories...]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Count sensor data readings that match the specified filters." << std::endl;
    std::cerr << "Accepts the same filtering options as 'transform'." << std::endl;
    std::cerr << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  -if, --input-format <fmt> Input format for stdin: json or csv (default: json)" << std::endl;
    std::cerr << "  -f, --follow              Follow mode: continuously monitor file/stdin for new data" << std::endl;
    std::cerr << "  -r, --recursive           Recursively process subdirectories" << std::endl;
    std::cerr << "  -v                        Verbose output (show progress)" << std::endl;
    std::cerr << "  -V                        Very verbose output (show detailed progress)" << std::endl;
    std::cerr << "  -e, --extension <ext>     Filter files by extension (e.g., .out or out)" << std::endl;
    std::cerr << "  -d, --depth <n>           Maximum recursion depth (0 = current dir only)" << std::endl;
    std::cerr << "  --not-empty <column>      Skip rows where column is empty (can be used multiple times)" << std::endl;
    std::cerr << "  --only-value <col:val>    Only include rows where column has specific value" << std::endl;
    std::cerr << "  --exclude-value <col:val> Exclude rows where column has specific value" << std::endl;
    std::cerr << "  --remove-errors           Remove error readings (DS18B20 value=85 or -127)" << std::endl;
    std::cerr << "  --remove-empty-json       Remove empty JSON input lines (e.g., [{}], [])" << std::endl;
    std::cerr << "  --clean                   Shorthand for --remove-empty-json --not-empty value --remove-errors" << std::endl;
    std::cerr << "  --min-date <date>         Filter readings after this date" << std::endl;
    std::cerr << "  --max-date <date>         Filter readings before this date" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Examples:" << std::endl;
    std::cerr << "  " << progName << " count sensor1.out" << std::endl;
    std::cerr << "  " << progName << " count < sensor1.out" << std::endl;
    std::cerr << "  " << progName << " count -r -e .out /path/to/logs" << std::endl;
    std::cerr << "  " << progName << " count --remove-errors sensor1.out" << std::endl;
    std::cerr << "  " << progName << " count --only-value type:temperature sensor1.out" << std::endl;
    std::cerr << "  " << progName << " count --clean sensor.out  # exclude empty values" << std::endl;
    std::cerr << "  " << progName << " count --follow sensor.out" << std::endl;
    std::cerr << "  tail -f sensor.out | " << progName << " count --follow" << std::endl;
}
