#include "data_counter.h"
#include <fstream>
#include <algorithm>

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

// ===== Constructor =====

DataCounter::DataCounter(int argc, char* argv[]) {
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

    // Parse common flags and collect files
    CommonArgParser parser;
    if (!parser.parse(argc, argv)) {
        exit(1);
    }

    copyFromParser(parser);

    // Check for unknown options
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg[0] == '-' && arg != "-r" && arg != "--recursive" && 
            arg != "-v" && arg != "-V" && arg != "-f" && arg != "--format" &&
            arg != "-e" && arg != "--extension" && arg != "-d" && arg != "--depth" &&
            arg != "--not-empty" && arg != "--only-value" && arg != "--exclude-value" &&
            arg != "--remove-errors" && arg != "--remove-empty-json" &&
            arg != "--min-date" && arg != "--max-date" &&
            arg != "-h" && arg != "--help") {
            if (i > 1) {
                std::string prev = argv[i-1];
                if (prev == "-f" || prev == "--format" || prev == "-e" || prev == "--extension" ||
                    prev == "-d" || prev == "--depth" || prev == "--not-empty" ||
                    prev == "--only-value" || prev == "--exclude-value" ||
                    prev == "--min-date" || prev == "--max-date") {
                    continue;
                }
            }
            std::cerr << "Error: Unknown option '" << arg << "'" << std::endl;
            printCountUsage(argv[0]);
            exit(1);
        }
    }
}

// ===== Main count method =====

void DataCounter::count() {
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
    std::cerr << "  -f, --format <fmt>        Input format for stdin: json or csv (default: json)" << std::endl;
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
    std::cerr << "  --min-date <date>         Filter readings after this date" << std::endl;
    std::cerr << "  --max-date <date>         Filter readings before this date" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Examples:" << std::endl;
    std::cerr << "  " << progName << " count sensor1.out" << std::endl;
    std::cerr << "  " << progName << " count < sensor1.out" << std::endl;
    std::cerr << "  " << progName << " count -r -e .out /path/to/logs" << std::endl;
    std::cerr << "  " << progName << " count --remove-errors sensor1.out" << std::endl;
    std::cerr << "  " << progName << " count --only-value type:temperature sensor1.out" << std::endl;
}
