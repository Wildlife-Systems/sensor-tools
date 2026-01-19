#include "data_counter.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>

#include "csv_parser.h"
#include "json_parser.h"
#include "file_utils.h"
#include "file_collector.h"
#include "data_reader.h"

// ===== Private methods =====

long long DataCounter::countFromFile(const std::string& filename) {
    if (verbosity >= 1) {
        std::cerr << "Counting: " << filename << std::endl;
    }

    long long count = 0;
    std::map<std::string, long long> localValueCounts;  // Local counts for thread safety
    DataReader reader(minDate, maxDate, verbosity, FileUtils::isCsvFile(filename) ? "csv" : "json", tailLines);
    
    reader.processFile(filename, [&](const Reading& reading, int /*lineNum*/, const std::string& /*source*/) {
        // Skip empty JSON arrays/objects if removeEmptyJson is set
        if (reading.empty()) return;
        if (shouldIncludeReading(reading)) {
            count++;
            if (!byColumn.empty()) {
                auto it = reading.find(byColumn);
                std::string value = (it != reading.end()) ? it->second : "(missing)";
                localValueCounts[value]++;
            }
        }
    });

    // Merge local counts into shared valueCounts with mutex
    if (!byColumn.empty() && !localValueCounts.empty()) {
        std::lock_guard<std::mutex> lock(valueCountsMutex);
        for (const auto& pair : localValueCounts) {
            valueCounts[pair.first] += pair.second;
        }
    }

    return count;
}

long long DataCounter::countFromStdin() {
    if (verbosity >= 1) {
        std::cerr << "Reading from stdin..." << std::endl;
    }

    long long count = 0;
    std::map<std::string, long long> localValueCounts;  // Local counts for thread safety
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

            Reading reading;
            for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                reading[csvHeaders[i]] = fields[i];
            }

            if (shouldIncludeReading(reading)) {
                count++;
                if (!byColumn.empty()) {
                    auto it = reading.find(byColumn);
                    std::string value = (it != reading.end()) ? it->second : "(missing)";
                    localValueCounts[value]++;
                }
            }
        }
    } else {
        // JSON format
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;

            auto readings = JsonParser::parseJsonLine(line);
            
            if (removeEmptyJson && areAllReadingsEmpty(readings)) continue;

            for (const auto& reading : readings) {
                if (reading.empty()) continue;
                if (shouldIncludeReading(reading)) {
                    count++;
                    if (!byColumn.empty()) {
                        auto it = reading.find(byColumn);
                        std::string value = (it != reading.end()) ? it->second : "(missing)";
                        localValueCounts[value]++;
                    }
                }
            }
        }
    }

    // Merge local counts into shared valueCounts with mutex
    if (!byColumn.empty() && !localValueCounts.empty()) {
        std::lock_guard<std::mutex> lock(valueCountsMutex);
        for (const auto& pair : localValueCounts) {
            valueCounts[pair.first] += pair.second;
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

                Reading reading;
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
                
                if (removeEmptyJson && areAllReadingsEmpty(readings)) continue;

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

            Reading reading;
            for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                reading[csvHeaders[i]] = fields[i];
            }

            if (shouldIncludeReading(reading)) {
                count++;
            }
        } else {
            auto readings = JsonParser::parseJsonLine(line);
            
            if (removeEmptyJson && areAllReadingsEmpty(readings)) continue;

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

                Reading reading;
                for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                    reading[csvHeaders[i]] = fields[i];
                }

                if (shouldIncludeReading(reading)) {
                    count++;
                    std::cout << count << std::endl;
                }
            } else {
                auto readings = JsonParser::parseJsonLine(line);
                
                if (removeEmptyJson && areAllReadingsEmpty(readings)) continue;

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

DataCounter::DataCounter(int argc, char* argv[]) : followMode(false), byColumn(""), outputFormat("human") {
    // Check for help flag first
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printCountUsage(argv[0]);
            exit(0);
        }
    }

    // Parse count-specific arguments and build filtered argv for CommonArgParser
    std::vector<char*> filteredArgv;
    for (int i = 0; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--follow" || arg == "-f") {
            followMode = true;
            continue;
        } else if ((arg == "--by-column" || arg == "-b") && i + 1 < argc) {
            byColumn = argv[i + 1];
            i++; // Skip the value
            continue;
        } else if ((arg == "--output-format" || arg == "-of") && i + 1 < argc) {
            outputFormat = argv[i + 1];
            if (outputFormat != "human" && outputFormat != "csv" && outputFormat != "json") {
                std::cerr << "Error: --output-format must be 'human', 'csv', or 'json'" << std::endl;
                exit(1);
            }
            i++; // Skip the value
            continue;
        }
        filteredArgv.push_back(argv[i]);
    }

    // Parse common flags and collect files using filtered argv
    CommonArgParser parser;
    if (!parser.parse(static_cast<int>(filteredArgv.size()), filteredArgv.data())) {
        exit(1);
    }

    copyFromParser(parser);

    // Check for unknown options using filtered argv
    std::string unknownOpt = CommonArgParser::checkUnknownOptions(
        static_cast<int>(filteredArgv.size()), filteredArgv.data());
    if (!unknownOpt.empty()) {
        std::cerr << "Error: Unknown option '" << unknownOpt << "'" << std::endl;
        printCountUsage(argv[0]);
        exit(1);
    }
}

// ===== Main count method =====

void DataCounter::count() {
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
        // Use parallel processing for multiple files
        totalCount = processFilesParallel<long long>(
            inputFiles,
            [this](const std::string& file) { return countFromFile(file); },
            [](long long& acc, long long val) { acc += val; },
            0LL,
            4  // numThreads
        );
    }

    if (!byColumn.empty()) {
        // Convert to vector and sort by count descending
        std::vector<std::pair<std::string, long long>> results(valueCounts.begin(), valueCounts.end());
        std::sort(results.begin(), results.end(), 
            [](const std::pair<std::string, long long>& a, const std::pair<std::string, long long>& b) {
                return a.second > b.second;  // Sort by count descending
            });
        
        if (outputFormat == "json") {
            std::cout << "[";
            bool first = true;
            for (const auto& pair : results) {
                if (!first) std::cout << ",";
                first = false;
                std::cout << "{\"" << byColumn << "\":\"" << pair.first 
                          << "\",\"count\":" << pair.second << "}";
            }
            std::cout << "]\n";
        } else if (outputFormat == "csv") {
            std::cout << byColumn << ",count\n";
            for (const auto& pair : results) {
                std::cout << pair.first << "," << pair.second << "\n";
            }
        } else {
            // Human-readable format (default)
            // Find max value width for alignment
            size_t maxValueWidth = byColumn.length();
            for (const auto& pair : results) {
                maxValueWidth = std::max(maxValueWidth, pair.first.length());
            }
            
            std::cout << "Counts by " << byColumn << ":\n\n";
            std::cout << std::left;
            std::cout.width(maxValueWidth + 2);
            std::cout << byColumn;
            std::cout << "Count\n";
            std::cout << std::string(maxValueWidth + 2 + 10, '-') << "\n";
            
            for (const auto& pair : results) {
                std::cout.width(maxValueWidth + 2);
                std::cout << pair.first;
                std::cout << pair.second << "\n";
            }
            
            std::cout << "\nTotal: " << totalCount << " reading(s)\n";
        }
    } else {
        std::cout << totalCount << std::endl;
    }
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
    std::cerr << "  -of, --output-format <fmt> Output format: human (default), csv, or json" << std::endl;
    std::cerr << "  -f, --follow              Follow mode: continuously monitor file/stdin for new data" << std::endl;
    std::cerr << "  -b, --by-column <col>     Show counts per value in the specified column" << std::endl;
    std::cerr << "  -r, --recursive           Recursively process subdirectories" << std::endl;
    std::cerr << "  -v                        Verbose output (show progress)" << std::endl;
    std::cerr << "  -V                        Very verbose output (show detailed progress)" << std::endl;
    std::cerr << "  -e, --extension <ext>     Filter files by extension (e.g., .out or out)" << std::endl;
    std::cerr << "  -d, --depth <n>           Maximum recursion depth (0 = current dir only)" << std::endl;
    std::cerr << "  --not-empty <column>      Skip rows where column is empty (can be used multiple times)" << std::endl;
    std::cerr << "  --not-null <column>       Skip rows where column is 'null' (can be used multiple times)" << std::endl;
    std::cerr << "  --only-value <col:val>    Only include rows where column has specific value" << std::endl;
    std::cerr << "  --exclude-value <col:val> Exclude rows where column has specific value" << std::endl;
    std::cerr << "  --allowed-values <column> <values|file> Only include rows where column is in allowed values" << std::endl;
    std::cerr << "  --remove-errors           Remove error readings (DS18B20 value=85 or -127)" << std::endl;
    std::cerr << "  --remove-empty-json       Remove empty JSON input lines (e.g., [{}], [])" << std::endl;
    std::cerr << "  --clean                   Shorthand for --remove-empty-json --not-empty value --remove-errors --not-null value --not-null sensor_id" << std::endl;
    std::cerr << "  --min-date <date>         Filter readings after this date" << std::endl;
    std::cerr << "  --max-date <date>         Filter readings before this date" << std::endl;
    std::cerr << "  --tail <n>                Only read the last n lines from each file" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Examples:" << std::endl;
    std::cerr << "  " << progName << " count sensor1.out" << std::endl;
    std::cerr << "  " << progName << " count < sensor1.out" << std::endl;
    std::cerr << "  " << progName << " count -r -e .out /path/to/logs" << std::endl;
    std::cerr << "  " << progName << " count --remove-errors sensor1.out" << std::endl;
    std::cerr << "  " << progName << " count --only-value type:temperature sensor1.out" << std::endl;
    std::cerr << "  " << progName << " count --allowed-values sensor_id allowed_sensors.txt sensor1.out" << std::endl;
    std::cerr << "  " << progName << " count --clean sensor.out  # exclude empty values" << std::endl;
    std::cerr << "  " << progName << " count --by-column sensor sensor1.out  # count per sensor" << std::endl;
    std::cerr << "  " << progName << " count --follow sensor.out" << std::endl;
    std::cerr << "  tail -f sensor.out | " << progName << " count --follow" << std::endl;
}
