#include "distinct_lister.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <future>

#include "csv_parser.h"
#include "json_parser.h"
#include "file_utils.h"
#include "file_collector.h"
#include "data_reader.h"

// ===== Private methods =====

void DistinctLister::collectFromFile(const std::string& filename) {
    if (verbosity >= 1) {
        std::cerr << "Processing: " << filename << std::endl;
    }

    std::set<std::string> localValues;
    std::map<std::string, long long> localCounts;
    DataReader reader = createDataReader();
    
    reader.processFile(filename, [&](const Reading& reading, int /*lineNum*/, const std::string& /*source*/) {
        auto it = reading.find(columnName);
        if (it != reading.end() && !it->second.empty()) {
            localValues.insert(it->second);
            if (showCounts) {
                localCounts[it->second]++;
            }
        }
    });

    // Merge local values into shared set with mutex
    if (!localValues.empty()) {
        std::lock_guard<std::mutex> lock(valuesMutex);
        distinctValues.insert(localValues.begin(), localValues.end());
        if (showCounts) {
            for (const auto& pair : localCounts) {
                valueCounts[pair.first] += pair.second;
            }
        }
    }
}

void DistinctLister::collectFromStdin() {
    if (verbosity >= 1) {
        std::cerr << "Reading from stdin..." << std::endl;
    }

    DataReader reader = createDataReader();
    
    reader.processStdin([&](const Reading& reading, int /*lineNum*/, const std::string& /*source*/) {
        auto it = reading.find(columnName);
        if (it != reading.end() && !it->second.empty()) {
            distinctValues.insert(it->second);
            if (showCounts) {
                valueCounts[it->second]++;
            }
        }
    });
}

void DistinctLister::outputResults() {
    if (outputFormat == "json") {
        std::cout << "[";
        bool first = true;
        if (showCounts) {
            // Sort by count descending
            std::vector<std::pair<std::string, long long>> sorted(valueCounts.begin(), valueCounts.end());
            std::sort(sorted.begin(), sorted.end(), 
                [](const std::pair<std::string, long long>& a, const std::pair<std::string, long long>& b) { return a.second > b.second; });
            for (const auto& pair : sorted) {
                if (!first) std::cout << ",";
                first = false;
                std::cout << "\n  {\"value\": \"" << pair.first << "\", \"count\": " << pair.second << "}";
            }
        } else {
            for (const auto& value : distinctValues) {
                if (!first) std::cout << ",";
                first = false;
                std::cout << "\n  \"" << value << "\"";
            }
        }
        std::cout << "\n]" << std::endl;
    } else if (outputFormat == "csv") {
        if (showCounts) {
            std::cout << "value,count" << std::endl;
            // Sort by count descending
            std::vector<std::pair<std::string, long long>> sorted(valueCounts.begin(), valueCounts.end());
            std::sort(sorted.begin(), sorted.end(), 
                [](const std::pair<std::string, long long>& a, const std::pair<std::string, long long>& b) { return a.second > b.second; });
            for (const auto& pair : sorted) {
                // Quote value if it contains comma, newline, or quote
                if (pair.first.find_first_of(",\"\n") != std::string::npos) {
                    std::string escaped = pair.first;
                    size_t pos = 0;
                    while ((pos = escaped.find('"', pos)) != std::string::npos) {
                        escaped.replace(pos, 1, "\"\"");
                        pos += 2;
                    }
                    std::cout << "\"" << escaped << "\"," << pair.second << std::endl;
                } else {
                    std::cout << pair.first << "," << pair.second << std::endl;
                }
            }
        } else {
            std::cout << "value" << std::endl;
            for (const auto& value : distinctValues) {
                // Quote value if it contains comma, newline, or quote
                if (value.find_first_of(",\"\n") != std::string::npos) {
                    std::string escaped = value;
                    size_t pos = 0;
                    while ((pos = escaped.find('"', pos)) != std::string::npos) {
                        escaped.replace(pos, 1, "\"\"");
                        pos += 2;
                    }
                    std::cout << "\"" << escaped << "\"" << std::endl;
                } else {
                    std::cout << value << std::endl;
                }
            }
        }
    } else {
        // Plain text output
        if (showCounts) {
            // Sort by count descending
            std::vector<std::pair<std::string, long long>> sorted(valueCounts.begin(), valueCounts.end());
            std::sort(sorted.begin(), sorted.end(), 
                [](const std::pair<std::string, long long>& a, const std::pair<std::string, long long>& b) { return a.second > b.second; });
            for (const auto& pair : sorted) {
                std::cout << pair.second << "\t" << pair.first << std::endl;
            }
        } else {
            for (const auto& value : distinctValues) {
                std::cout << value << std::endl;
            }
        }
    }
}

// ===== Constructor =====

DistinctLister::DistinctLister(int argc, char* argv[]) 
    : outputFormat("plain"), showCounts(false) {
    
    // Check for help flag first
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printDistinctUsage(argv[0]);
            exit(0);
        }
    }

    // Find the column name (first positional argument)
    int columnArgIndex = -1;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg[0] != '-') {
            // Check if this looks like a file/directory path
            // Column names shouldn't contain path separators or common extensions
            if (arg.find('/') == std::string::npos && 
                arg.find('\\') == std::string::npos &&
                arg.find(".json") == std::string::npos &&
                arg.find(".csv") == std::string::npos) {
                columnName = arg;
                columnArgIndex = i;
                break;
            }
        }
    }

    if (columnName.empty()) {
        std::cerr << "Error: Column name is required" << std::endl;
        printDistinctUsage(argv[0]);
        exit(1);
    }

    // Parse distinct-specific arguments and build filtered argv for CommonArgParser
    std::vector<char*> filteredArgv;
    for (int i = 0; i < argc; ++i) {
        if (i == columnArgIndex) {
            continue;  // Skip column name
        }
        std::string arg = argv[i];
        if ((arg == "--output-format" || arg == "-of") && i + 1 < argc) {
            outputFormat = argv[i + 1];
            if (outputFormat != "plain" && outputFormat != "csv" && outputFormat != "json") {
                std::cerr << "Error: --output-format must be 'plain', 'csv', or 'json'" << std::endl;
                exit(1);
            }
            i++; // Skip the value
            continue;
        } else if (arg == "--counts" || arg == "-c") {
            showCounts = true;
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
        printDistinctUsage(argv[0]);
        exit(1);
    }
}

// ===== Public methods =====

void DistinctLister::listDistinct() {
    if (hasInputFiles) {
        // Multi-threaded file processing
        size_t numThreads = std::min(inputFiles.size(), static_cast<size_t>(8));
        size_t filesPerThread = std::max(size_t(1), inputFiles.size() / numThreads);
        std::vector<std::future<void>> futures;
        futures.reserve(numThreads);
        
        for (size_t i = 0; i < inputFiles.size(); i += filesPerThread) {
            size_t end = std::min(i + filesPerThread, inputFiles.size());
            
            futures.push_back(std::async(std::launch::async, [this, i, end]() {
                for (size_t j = i; j < end; ++j) {
                    collectFromFile(inputFiles[j]);
                }
            }));
        }
        
        for (auto& f : futures) {
            f.wait();
        }
    } else {
        collectFromStdin();
    }
    
    if (verbosity >= 1) {
        std::cerr << "Found " << distinctValues.size() << " distinct values" << std::endl;
    }
    
    outputResults();
}

void DistinctLister::printDistinctUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " distinct <column> [options] [files...]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "List unique values in a specified column." << std::endl;
    std::cerr << std::endl;
    std::cerr << "Arguments:" << std::endl;
    std::cerr << "  <column>           Column name to get distinct values from" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Output options:" << std::endl;
    std::cerr << "  -of, --output-format <format>  Output format: plain (default), csv, json" << std::endl;
    std::cerr << "  -c, --counts                   Include count for each value" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Common options:" << std::endl;
    std::cerr << "  -r, --recursive         Process directories recursively" << std::endl;
    std::cerr << "  -d, --depth <n>         Maximum recursion depth" << std::endl;
    std::cerr << "  -e, --ext <extension>   Filter by file extension (without dot)" << std::endl;
    std::cerr << "  -if, --input-format <format>   Input format: json (default), csv" << std::endl;
    std::cerr << "  -v, --verbose           Increase verbosity" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Filter options:" << std::endl;
    std::cerr << "  --clean                 Remove readings with errors and enable --unique" << std::endl;
    std::cerr << "  --unique                Only output unique rows (removes duplicates)" << std::endl;
    std::cerr << "  --after <date>          Only include readings after date" << std::endl;
    std::cerr << "  --before <date>         Only include readings before date" << std::endl;
    std::cerr << "  --only-value <col:val>  Only include readings where col=val" << std::endl;
    std::cerr << "  --exclude-value <col:val>  Exclude readings where col=val" << std::endl;
    std::cerr << "  --not-empty <col>       Exclude readings where col is empty" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Examples:" << std::endl;
    std::cerr << "  " << progName << " distinct sensor data/*.json" << std::endl;
    std::cerr << "  " << progName << " distinct sensor_id -r ~/data/" << std::endl;
    std::cerr << "  " << progName << " distinct node_id --clean -c" << std::endl;
    std::cerr << "  cat data.json | " << progName << " distinct sensor" << std::endl;
}
