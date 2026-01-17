#ifndef STATS_ANALYSER_H
#define STATS_ANALYSER_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>

#include "common_arg_parser.h"
#include "data_reader.h"

class StatsAnalyser {
private:
    std::vector<std::string> inputFiles;
    bool recursive;
    std::string extensionFilter;
    int maxDepth;
    int verbosity;
    std::string inputFormat;  // Input format: "json" or "csv" (default: json for stdin)
    std::string columnFilter;  // Specific column to analyze (empty = analyze all numeric columns)
    long long minDate;  // Minimum timestamp (Unix epoch)
    long long maxDate;  // Maximum timestamp (Unix epoch)
    std::map<std::string, std::vector<double>> columnData;  // column name -> values
    
    bool isNumeric(const std::string& str) {
        if (str.empty()) return false;
        try {
            size_t pos = 0;
            std::stod(str, &pos);
            return pos == str.length();
        } catch (...) {
            return false;
        }
    }
    
    void collectDataFromReading(const std::map<std::string, std::string>& reading) {
        for (const auto& pair : reading) {
            // Skip if we're filtering by column and this isn't it
            if (!columnFilter.empty() && pair.first != columnFilter) continue;
            
            // Try to parse as numeric
            if (isNumeric(pair.second)) {
                double value = std::stod(pair.second);
                columnData[pair.first].push_back(value);
            }
        }
    }
    
    double calculateMedian(const std::vector<double>& values) {
        if (values.empty()) return 0.0;
        std::vector<double> sorted = values;  // Make copy for sorting
        std::sort(sorted.begin(), sorted.end());
        size_t n = sorted.size();
        if (n % 2 == 0) {
            return (sorted[n/2 - 1] + sorted[n/2]) / 2.0;
        } else {
            return sorted[n/2];
        }
    }
    
    double calculateStdDev(const std::vector<double>& values, double mean) {
        if (values.size() <= 1) return 0.0;
        double sum = 0.0;
        for (double v : values) {
            sum += (v - mean) * (v - mean);
        }
        return std::sqrt(sum / (values.size() - 1));
    }
    
public:
    StatsAnalyser(int argc, char* argv[]) : columnFilter("value") {
        // Check for help flag first
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--help" || arg == "-h") {
                printStatsUsage(argv[0]);
                exit(0);
            }
        }
        
        CommonArgParser parser;
        if (!parser.parse(argc, argv)) {
            exit(1);
        }
        
        // Parse StatsAnalyser-specific options (-c/--column)
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "-c" || arg == "--column") {
                if (i + 1 < argc) {
                    ++i;
                    columnFilter = argv[i];
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    exit(1);
                }
            }
        }
        
        // Check for unknown options
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg[0] == '-' && arg != "-r" && arg != "--recursive" && 
                arg != "-v" && arg != "-V" && arg != "-f" && arg != "--format" &&
                arg != "-e" && arg != "--extension" && arg != "-d" && arg != "--depth" &&
                arg != "-c" && arg != "--column" &&
                arg != "--min-date" && arg != "--max-date" &&
                arg != "-h" && arg != "--help") {
                if (i > 1) {
                    std::string prev = argv[i-1];
                    if (prev == "-f" || prev == "--format" || prev == "-e" || prev == "--extension" ||
                        prev == "-d" || prev == "--depth" || prev == "-c" || prev == "--column" ||
                        prev == "--min-date" || prev == "--max-date") {
                        continue;
                    }
                }
                std::cerr << "Error: Unknown option '" << arg << "'" << std::endl;
                printStatsUsage(argv[0]);
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
    
    void analyze() {
        DataReader reader(minDate, maxDate, verbosity, inputFormat);
        
        if (inputFiles.empty()) {
            // Reading from stdin
            reader.processStdin([&](const std::map<std::string, std::string>& reading, int lineNum, const std::string& source) {
                collectDataFromReading(reading);
            });
        } else {
            printCommonVerboseInfo("Analyzing", verbosity, recursive, extensionFilter, maxDepth, inputFiles.size());
            
            for (const auto& file : inputFiles) {
                reader.processFile(file, [&](const std::map<std::string, std::string>& reading, int lineNum, const std::string& source) {
                    collectDataFromReading(reading);
                });
            }
        }
        
        // Print statistics
        if (columnData.empty()) {
            std::cout << "No numeric data found" << std::endl;
        } else {
            std::cout << "Statistics:" << std::endl;
            std::cout << std::endl;
            
            for (const auto& pair : columnData) {
                const std::string& colName = pair.first;
                const std::vector<double>& values = pair.second;
                
                if (values.empty()) continue;
                
                double min = *std::min_element(values.begin(), values.end());
                double max = *std::max_element(values.begin(), values.end());
                double sum = 0.0;
                for (double v : values) sum += v;
                double mean = sum / values.size();
                double median = calculateMedian(values);
                double stddev = calculateStdDev(values, mean);
                
                std::cout << colName << ":" << std::endl;
                std::cout << "  Count:  " << values.size() << std::endl;
                std::cout << "  Min:    " << min << std::endl;
                std::cout << "  Max:    " << max << std::endl;
                std::cout << "  Mean:   " << mean << std::endl;
                std::cout << "  Median: " << median << std::endl;
                std::cout << "  StdDev: " << stddev << std::endl;
                std::cout << std::endl;
            }
        }
    }
    
    static void printStatsUsage(const char* progName) {
        std::cerr << "Usage: " << progName << " stats [options] [<input_file(s)_or_directory(ies)>]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Calculate statistics for numeric sensor data." << std::endl;
        std::cerr << "Shows min, max, mean, median, and standard deviation for numeric columns." << std::endl;
        std::cerr << "If no input files are specified, reads from stdin (assumes JSON format unless -f is used)." << std::endl;
        std::cerr << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << "  -c, --column <name>       Analyze only this column (default: value)" << std::endl;
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
        std::cerr << "  " << progName << " stats sensor1.out" << std::endl;
        std::cerr << "  " << progName << " stats < sensor1.out" << std::endl;
        std::cerr << "  " << progName << " stats -c value < sensor1.out" << std::endl;
        std::cerr << "  " << progName << " stats -f csv < sensor1.csv" << std::endl;
        std::cerr << "  cat sensor1.out | " << progName << " stats" << std::endl;
        std::cerr << "  " << progName << " stats -r -e .out /path/to/logs/" << std::endl;
        std::cerr << "  " << progName << " stats sensor1.csv sensor2.out" << std::endl;
    }
};

#endif // STATS_ANALYSER_H
