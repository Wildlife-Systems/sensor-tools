#include "latest_finder.h"
#include "common_arg_parser.h"
#include "data_reader.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstring>

LatestFinder::LatestFinder(int argc, char* argv[]) : limitRows(0), outputFormat("human") {
    // Check for help flag first
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            usage();
            exit(0);
        }
    }
    
    // Parse custom arguments first and build filtered argv
    std::vector<char*> filteredArgv;
    for (int i = 0; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-n" && i + 1 < argc) {
            limitRows = std::atoi(argv[i + 1]);
            i++; // Skip the value
            continue;
        }
        if ((arg == "-of" || arg == "--output-format") && i + 1 < argc) {
            outputFormat = argv[i + 1];
            i++; // Skip the value
            continue;
        }
        filteredArgv.push_back(argv[i]);
    }
    
    CommonArgParser parser;
    if (!parser.parse(static_cast<int>(filteredArgv.size()), filteredArgv.data())) {
        exit(1);
    }
    
    // Check for unknown options
    std::string unknownOpt = CommonArgParser::checkUnknownOptions(
        static_cast<int>(filteredArgv.size()), filteredArgv.data());
    if (!unknownOpt.empty()) {
        std::cerr << "Error: Unknown option '" << unknownOpt << "'" << std::endl;
        usage();
        exit(1);
    }
    
    copyFromParser(parser);
}

void LatestFinder::usage() {
    std::cerr << "Usage: sensor-data latest [OPTIONS] <file(s)/directory>\n"
              << "  Outputs the latest timestamp for each sensor_id\n"
              << "\nOptions:\n"
              << "  -n <num>           Limit output rows (positive = first n, negative = last n)\n"
              << "  -of, --output-format <fmt>  Output format: human (default), csv, or json\n"
              << "  --min-date <date>  Only consider readings after this date\n"
              << "  --max-date <date>  Only consider readings before this date\n"
              << "  -if, --input-format <fmt>  Input format: json (default) or csv\n"
              << "  --tail <n>         Only read last n lines from each file\n"
              << "  --tail-column-value <col:val> <n>  Return last n rows where column=value\n"
              << "  -v, --verbose      Show verbose output\n"
              << "  -h, --help         Show this help message\n"
              << "\nOutput columns: sensor_id, unix_timestamp, iso_date\n";
}

int LatestFinder::main() {
    if (inputFiles.empty()) {
        std::cerr << "Error: No input files specified\n";
        usage();
        return 1;
    }
    
    printCommonVerboseInfo("latest", verbosity, recursive, extensionFilter, maxDepth, inputFiles.size());
    
    // Process files in parallel, each thread builds its own local map
    auto processFile = [this](const std::string& file) -> std::map<std::string, SensorLatest> {
        std::map<std::string, SensorLatest> localLatest;
        DataReader reader = createDataReader();
        
        if (verbosity > 0) {
            std::cerr << "Processing: " << file << "\n";
        }
        
        reader.processFile(file, [&](const Reading& reading, int, const std::string&) {
            // Filtering already done by DataReader
            
            // Get sensor_id
            auto sensorIt = reading.find("sensor_id");
            if (sensorIt == reading.end() || sensorIt->second.empty()) {
                return;
            }
            std::string sensorId = sensorIt->second;
            
            // Get timestamp
            long long ts = DateUtils::getTimestamp(reading);
            if (ts <= 0) return;
            
            // Update if this is the latest for this sensor
            auto& entry = localLatest[sensorId];
            if (ts > entry.timestamp) {
                entry.sensorId = sensorId;
                entry.timestamp = ts;
            }
        });
        
        return localLatest;
    };
    
    // Combine function: merge maps keeping latest timestamp per sensor
    auto combineLatest = [](std::map<std::string, SensorLatest>& combined, 
                            const std::map<std::string, SensorLatest>& local) {
        for (const auto& [sensorId, data] : local) {
            auto& existing = combined[sensorId];
            if (data.timestamp > existing.timestamp) {
                existing = data;
            }
        }
    };
    
    std::map<std::string, SensorLatest> latestBySensor = 
        processFilesParallel(inputFiles, processFile, combineLatest, 
                             std::map<std::string, SensorLatest>());
    
    // Convert to vector for sorting and limiting
    std::vector<SensorLatest> results;
    for (const auto& [sensorId, data] : latestBySensor) {
        results.push_back(data);
    }
    
    // Sort by sensor_id (natural order from map)
    std::sort(results.begin(), results.end(), [](const SensorLatest& a, const SensorLatest& b) {
        return a.sensorId < b.sensorId;
    });
    
    // Apply -n limiting
    size_t startIdx = 0;
    size_t endIdx = results.size();
    
    if (limitRows != 0) {
        if (limitRows > 0) {
            // First n rows
            endIdx = std::min(static_cast<size_t>(limitRows), results.size());
        } else {
            // Last n rows (negative)
            size_t count = static_cast<size_t>(-limitRows);
            if (count < results.size()) {
                startIdx = results.size() - count;
            }
        }
    }
    
    // Output results
    if (outputFormat == "json") {
        std::cout << "[";
        bool first = true;
        for (size_t i = startIdx; i < endIdx; ++i) {
            const SensorLatest& entry = results[i];
            char isoDate[32];
            time_t t = static_cast<time_t>(entry.timestamp);
            std::strftime(isoDate, sizeof(isoDate), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
            
            if (!first) std::cout << ",";
            first = false;
            std::cout << "{\"sensor_id\":\"" << entry.sensorId 
                      << "\",\"timestamp\":" << entry.timestamp 
                      << ",\"iso_date\":\"" << isoDate << "\"}";
        }
        std::cout << "]\n";
    } else if (outputFormat == "csv") {
        std::cout << "sensor_id,timestamp,iso_date\n";
        for (size_t i = startIdx; i < endIdx; ++i) {
            const SensorLatest& entry = results[i];
            char isoDate[32];
            time_t t = static_cast<time_t>(entry.timestamp);
            std::strftime(isoDate, sizeof(isoDate), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
            std::cout << entry.sensorId << "," << entry.timestamp << "," << isoDate << "\n";
        }
    } else {
        // Human-readable format (default)
        // Find max sensor_id width for alignment
        size_t maxIdWidth = 9; // "sensor_id"
        for (size_t i = startIdx; i < endIdx; ++i) {
            maxIdWidth = std::max(maxIdWidth, results[i].sensorId.length());
        }
        
        std::cout << "Latest readings by sensor:\n\n";
        std::cout << std::left;
        std::cout.width(maxIdWidth + 2);
        std::cout << "Sensor ID";
        std::cout.width(14);
        std::cout << "Timestamp";
        std::cout << "Date/Time\n";
        std::cout << std::string(maxIdWidth + 2 + 14 + 19, '-') << "\n";
        
        for (size_t i = startIdx; i < endIdx; ++i) {
            const SensorLatest& entry = results[i];
            char isoDate[32];
            time_t t = static_cast<time_t>(entry.timestamp);
            std::strftime(isoDate, sizeof(isoDate), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
            
            std::cout.width(maxIdWidth + 2);
            std::cout << entry.sensorId;
            std::cout.width(14);
            std::cout << entry.timestamp;
            std::cout << isoDate << "\n";
        }
        
        std::cout << "\nTotal: " << (endIdx - startIdx) << " sensor(s)\n";
    }
    
    return 0;
}
