#include "stats_analyser.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <ctime>
#include <thread>
#include <iomanip>

#include "data_reader.h"

// ===== Private static methods =====

bool StatsAnalyser::isNumeric(const std::string& str) {
    if (str.empty()) return false;
    try {
        size_t pos = 0;
        std::stod(str, &pos);
        return pos == str.length();
    } catch (...) {
        return false;
    }
}

double StatsAnalyser::calculateMedian(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    size_t n = sorted.size();
    if (n % 2 == 0) {
        return (sorted[n/2 - 1] + sorted[n/2]) / 2.0;
    } else {
        return sorted[n/2];
    }
}

double StatsAnalyser::calculateStdDev(const std::vector<double>& values, double mean) {
    if (values.size() <= 1) return 0.0;
    double sum = 0.0;
    for (double v : values) {
        sum += (v - mean) * (v - mean);
    }
    return std::sqrt(sum / (values.size() - 1));
}

double StatsAnalyser::calculatePercentile(const std::vector<double>& sortedValues, double percentile) {
    if (sortedValues.empty()) return 0.0;
    if (sortedValues.size() == 1) return sortedValues[0];
    
    double index = (percentile / 100.0) * (sortedValues.size() - 1);
    size_t lower = static_cast<size_t>(index);
    size_t upper = lower + 1;
    double fraction = index - lower;
    
    if (upper >= sortedValues.size()) {
        return sortedValues[sortedValues.size() - 1];
    }
    
    return sortedValues[lower] + fraction * (sortedValues[upper] - sortedValues[lower]);
}

// ===== Private methods =====

void StatsAnalyser::collectDataFromReading(const std::map<std::string, std::string>& reading) {
    // Collect timestamp if present
    auto tsIt = reading.find("timestamp");
    if (tsIt != reading.end() && isNumeric(tsIt->second)) {
        long long ts = std::stoll(tsIt->second);
        timestamps.push_back(ts);
    }
    
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

// ===== Constructor =====

StatsAnalyser::StatsAnalyser(int argc, char* argv[]) : columnFilter("value"), followMode(false) {
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
    
    // Parse StatsAnalyser-specific options (-c/--column and --follow)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-c" || arg == "--column") {
            if (i + 1 < argc) {
                ++i;
                columnFilter = argv[i];
                // Special case: "all" means analyze all numeric columns
                if (columnFilter == "all") {
                    columnFilter = "";
                }
            } else {
                std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                exit(1);
            }
        } else if (arg == "--follow" || arg == "-f") {
            followMode = true;
        }
    }
    
    // Check for unknown options (stats-specific: -c/--column, -f/--follow)
    std::string unknownOpt = CommonArgParser::checkUnknownOptions(argc, argv, 
        {"-c", "--column", "-f", "--follow"});
    if (!unknownOpt.empty()) {
        std::cerr << "Error: Unknown option '" << unknownOpt << "'" << std::endl;
        printStatsUsage(argv[0]);
        exit(1);
    }
    
    copyFromParser(parser);
}

// ===== Print stats helper =====

void StatsAnalyser::printStats() {
    if (columnData.empty()) {
        std::cout << "No numeric data found" << std::endl;
        return;
    }
    
    std::cout << "Statistics:" << std::endl;
    std::cout << std::endl;
    
    // Time-based stats (if timestamps available)
    if (!timestamps.empty()) {
        std::vector<long long> sortedTs = timestamps;
        std::sort(sortedTs.begin(), sortedTs.end());
        
        long long firstTs = sortedTs.front();
        long long lastTs = sortedTs.back();
        long long duration = lastTs - firstTs;
        
        // Format timestamps as human-readable dates
        std::time_t firstTime = static_cast<std::time_t>(firstTs);
        std::time_t lastTime = static_cast<std::time_t>(lastTs);
        char firstBuf[64], lastBuf[64];
        std::strftime(firstBuf, sizeof(firstBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&firstTime));
        std::strftime(lastBuf, sizeof(lastBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&lastTime));
        
        std::cout << "Time Range:" << std::endl;
        std::cout << "  First:     " << firstBuf << " (" << firstTs << ")" << std::endl;
        std::cout << "  Last:      " << lastBuf << " (" << lastTs << ")" << std::endl;
        
        // Duration in human-readable format
        long long days = duration / 86400;
        long long hours = (duration % 86400) / 3600;
        long long minutes = (duration % 3600) / 60;
        long long seconds = duration % 60;
        std::cout << "  Duration:  ";
        if (days > 0) std::cout << days << "d ";
        if (hours > 0 || days > 0) std::cout << hours << "h ";
        if (minutes > 0 || hours > 0 || days > 0) std::cout << minutes << "m ";
        std::cout << seconds << "s (" << duration << " seconds)" << std::endl;
        
        // Readings rate
        if (duration > 0) {
            double readingsPerHour = (double)timestamps.size() / ((double)duration / 3600.0);
            double readingsPerDay = (double)timestamps.size() / ((double)duration / 86400.0);
            std::cout << "  Rate:      " << std::fixed << std::setprecision(2) 
                      << readingsPerHour << " readings/hour, "
                      << readingsPerDay << " readings/day" << std::endl;
            std::cout << std::defaultfloat;
        }
        
        // Gap detection
        if (sortedTs.size() > 1) {
            // Calculate typical interval
            std::vector<long long> intervals;
            for (size_t i = 1; i < sortedTs.size(); ++i) {
                intervals.push_back(sortedTs[i] - sortedTs[i-1]);
            }
            std::sort(intervals.begin(), intervals.end());
            long long medianInterval = intervals[intervals.size() / 2];
            
            // Find gaps (more than 3x the median interval)
            long long gapThreshold = medianInterval * 3;
            size_t gapCount = 0;
            long long maxGap = 0;
            for (long long interval : intervals) {
                if (interval > gapThreshold) {
                    gapCount++;
                    if (interval > maxGap) maxGap = interval;
                }
            }
            
            std::cout << std::endl;
            std::cout << "  Sampling:" << std::endl;
            std::cout << "    Typical interval: " << medianInterval << "s" << std::endl;
            std::cout << "    Gaps detected:    " << gapCount;
            if (gapCount > 0) {
                std::cout << " (max gap: " << maxGap << "s = ";
                long long gapHours = maxGap / 3600;
                long long gapMins = (maxGap % 3600) / 60;
                if (gapHours > 0) std::cout << gapHours << "h ";
                std::cout << gapMins << "m)";
            }
            std::cout << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    for (const auto& pair : columnData) {
        const std::string& colName = pair.first;
        const std::vector<double>& values = pair.second;
        
        if (values.empty()) continue;
        
        // Sort values for percentile calculations
        std::vector<double> sorted = values;
        std::sort(sorted.begin(), sorted.end());
        
        double min = sorted.front();
        double max = sorted.back();
        double sum = 0.0;
        for (double v : values) sum += v;
        double mean = sum / values.size();
        double stddev = calculateStdDev(values, mean);
        
        // Quartiles
        double q1 = calculatePercentile(sorted, 25);
        double median = calculatePercentile(sorted, 50);
        double q3 = calculatePercentile(sorted, 75);
        double iqr = q3 - q1;
        
        // Outliers (using 1.5 * IQR method)
        double lowerFence = q1 - 1.5 * iqr;
        double upperFence = q3 + 1.5 * iqr;
        size_t outlierCount = 0;
        for (double v : values) {
            if (v < lowerFence || v > upperFence) {
                outlierCount++;
            }
        }
        double outlierPercent = (values.size() > 0) ? (100.0 * outlierCount / values.size()) : 0.0;
        
        // Delta stats (differences between consecutive readings)
        double deltaMin = 0, deltaMax = 0, deltaSum = 0;
        std::vector<double> deltas;
        size_t deltaCount = 0;
        size_t maxJumpIndex = 1;  // Start at 1 (first valid index for a jump)
        if (values.size() > 1) {
            deltaMin = std::abs(values[1] - values[0]);
            deltaMax = deltaMin;
            for (size_t i = 1; i < values.size(); ++i) {
                double delta = std::abs(values[i] - values[i-1]);
                deltas.push_back(delta);
                deltaSum += delta;
                if (delta < deltaMin) deltaMin = delta;
                if (delta > deltaMax) {
                    deltaMax = delta;
                    maxJumpIndex = i;
                }
            }
            deltaCount = values.size() - 1;
        }
        double deltaMean = (deltaCount > 0) ? (deltaSum / deltaCount) : 0.0;
        
        // Volatility (std dev of deltas)
        double volatility = 0.0;
        if (deltas.size() > 1) {
            double deltaVarianceSum = 0.0;
            for (double d : deltas) {
                deltaVarianceSum += (d - deltaMean) * (d - deltaMean);
            }
            volatility = std::sqrt(deltaVarianceSum / (deltas.size() - 1));
        }
        
        std::cout << colName << ":" << std::endl;
        std::cout << "  Count:    " << values.size() << std::endl;
        std::cout << "  Min:      " << min << std::endl;
        std::cout << "  Max:      " << max << std::endl;
        std::cout << "  Range:    " << (max - min) << std::endl;
        std::cout << "  Mean:     " << mean << std::endl;
        std::cout << "  StdDev:   " << stddev << std::endl;
        std::cout << std::endl;
        std::cout << "  Quartiles:" << std::endl;
        std::cout << "    Q1 (25%):  " << q1 << std::endl;
        std::cout << "    Median:    " << median << std::endl;
        std::cout << "    Q3 (75%):  " << q3 << std::endl;
        std::cout << "    IQR:       " << iqr << std::endl;
        std::cout << std::endl;
        std::cout << "  Outliers (1.5*IQR):" << std::endl;
        std::cout << "    Count:     " << outlierCount << std::endl;
        std::cout << "    Percent:   " << std::fixed << std::setprecision(1) << outlierPercent << "%" << std::endl;
        std::cout << std::defaultfloat;
        
        if (values.size() > 1) {
            std::cout << std::endl;
            std::cout << "  Delta (consecutive changes):" << std::endl;
            std::cout << "    Min:       " << std::fixed << std::setprecision(4) << deltaMin << std::endl;
            std::cout << "    Max:       " << deltaMax << std::endl;
            std::cout << "    Mean:      " << deltaMean << std::endl;
            std::cout << "    Volatility:" << volatility << std::endl;
            std::cout << std::endl;
            std::cout << "  Max Jump:" << std::endl;
            std::cout << "    Size:      " << deltaMax << std::endl;
            std::cout << "    From:      " << values[maxJumpIndex - 1] << " -> " << values[maxJumpIndex] << std::endl;
            std::cout << std::defaultfloat;
        }
        
        std::cout << std::endl;
    }
}

// ===== Follow mode for stdin =====

void StatsAnalyser::analyzeStdinFollow() {
    if (verbosity >= 1) {
        std::cerr << "Reading from stdin with follow mode (format: " << inputFormat << ")..." << std::endl;
    }
    
    std::string line;
    std::vector<std::string> csvHeaders;
    bool headerParsed = false;
    
    // Print initial stats (will say "No numeric data found")
    printStats();
    std::cout << "---" << std::endl;
    
    while (true) {
        if (std::getline(std::cin, line)) {
            if (line.empty()) continue;
            
            bool dataUpdated = false;
            
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
                
                // Check all filters (date, only-value, exclude-value, etc.)
                if (!shouldIncludeReading(reading)) continue;
                
                collectDataFromReading(reading);
                dataUpdated = true;
            } else {
                // JSON format
                auto readings = JsonParser::parseJsonLine(line);
                for (const auto& reading : readings) {
                    if (reading.empty()) continue;
                    
                    // Check all filters (date, only-value, exclude-value, etc.)
                    if (!shouldIncludeReading(reading)) continue;
                    
                    collectDataFromReading(reading);
                    dataUpdated = true;
                }
            }
            
            if (dataUpdated) {
                printStats();
                std::cout << "---" << std::endl;
            }
        } else {
            // No more data available, wait briefly and try again
            std::cin.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void StatsAnalyser::analyzeFileFollow(const std::string& filename) {
    if (verbosity >= 1) {
        std::cerr << "Following file: " << filename << " (format: " << (FileUtils::isCsvFile(filename) ? "csv" : "json") << ")..." << std::endl;
    }
    
    std::ifstream infile(filename);
    if (!infile) {
        std::cerr << "Error: Cannot open file: " << filename << std::endl;
        return;
    }
    
    std::string line;
    std::vector<std::string> csvHeaders;
    bool isCSV = FileUtils::isCsvFile(filename);
    
    // Read existing content first
    if (isCSV) {
        if (std::getline(infile, line) && !line.empty()) {
            bool needMore = false;
            csvHeaders = CsvParser::parseCsvLine(infile, line, needMore);
        }
        
        while (std::getline(infile, line)) {
            if (line.empty()) continue;
            
            bool needMore = false;
            auto fields = CsvParser::parseCsvLine(infile, line, needMore);
            if (fields.empty()) continue;
            
            std::map<std::string, std::string> reading;
            for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                reading[csvHeaders[i]] = fields[i];
            }
            
            if (!shouldIncludeReading(reading)) continue;
            
            collectDataFromReading(reading);
        }
    } else {
        while (std::getline(infile, line)) {
            if (line.empty()) continue;
            
            auto readings = JsonParser::parseJsonLine(line);
            for (const auto& reading : readings) {
                if (reading.empty()) continue;
                
                if (!shouldIncludeReading(reading)) continue;
                
                collectDataFromReading(reading);
            }
        }
    }
    
    // Print initial stats
    printStats();
    std::cout << "---" << std::endl;
    
    // Now follow the file for new content
    infile.clear();  // Clear EOF flag
    
    while (true) {
        if (std::getline(infile, line)) {
            if (line.empty()) continue;
            
            bool dataUpdated = false;
            
            if (isCSV) {
                bool needMore = false;
                auto fields = CsvParser::parseCsvLine(infile, line, needMore);
                if (fields.empty()) continue;
                
                std::map<std::string, std::string> reading;
                for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                    reading[csvHeaders[i]] = fields[i];
                }
                
                if (!shouldIncludeReading(reading)) continue;
                
                collectDataFromReading(reading);
                dataUpdated = true;
            } else {
                auto readings = JsonParser::parseJsonLine(line);
                for (const auto& reading : readings) {
                    if (reading.empty()) continue;
                    
                    if (!shouldIncludeReading(reading)) continue;
                    
                    collectDataFromReading(reading);
                    dataUpdated = true;
                }
            }
            
            if (dataUpdated) {
                printStats();
                std::cout << "---" << std::endl;
            }
        } else {
            // No more data available, wait briefly and try again
            infile.clear();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

// Helper struct for parallel processing
struct LocalStatsData {
    std::map<std::string, std::vector<double>> columnData;
    std::vector<long long> timestamps;
};

// ===== Main analyze method =====

void StatsAnalyser::analyze() {
    if (inputFiles.empty() && followMode) {
        // Follow mode with stdin
        analyzeStdinFollow();
        return;
    }
    
    if (followMode && inputFiles.size() == 1) {
        // Follow mode with a single file
        analyzeFileFollow(inputFiles[0]);
        return;
    }
    
    if (followMode && inputFiles.size() > 1) {
        std::cerr << "Warning: --follow only supports a single file, using first file only" << std::endl;
        analyzeFileFollow(inputFiles[0]);
        return;
    }
    
    if (inputFiles.empty()) {
        DataReader reader(minDate, maxDate, verbosity, inputFormat, tailLines);
        auto collectData = [&](const std::map<std::string, std::string>& reading, int /*lineNum*/, const std::string& /*source*/) {
            if (!shouldIncludeReading(reading)) return;
            collectDataFromReading(reading);
        };
        reader.processStdin(collectData);
    } else {
        printCommonVerboseInfo("Analyzing", verbosity, recursive, extensionFilter, maxDepth, inputFiles.size());
        
        // Process files in parallel
        auto processFile = [this](const std::string& file) -> LocalStatsData {
            LocalStatsData local;
            DataReader reader(minDate, maxDate, verbosity, inputFormat, tailLines);
            
            reader.processFile(file, [&](const std::map<std::string, std::string>& reading, int, const std::string&) {
                if (!shouldIncludeReading(reading)) return;
                
                // Collect timestamp if present
                auto tsIt = reading.find("timestamp");
                if (tsIt != reading.end() && isNumeric(tsIt->second)) {
                    long long ts = std::stoll(tsIt->second);
                    local.timestamps.push_back(ts);
                }
                
                for (const auto& pair : reading) {
                    // Skip if we're filtering by column and this isn't it
                    if (!columnFilter.empty() && pair.first != columnFilter) continue;
                    
                    // Try to parse as numeric
                    if (isNumeric(pair.second)) {
                        double value = std::stod(pair.second);
                        local.columnData[pair.first].push_back(value);
                    }
                }
            });
            
            return local;
        };
        
        // Combine function: merge column data and timestamps
        auto combineStats = [](LocalStatsData& combined, const LocalStatsData& local) {
            // Merge column data
            for (const auto& pair : local.columnData) {
                auto& target = combined.columnData[pair.first];
                target.insert(target.end(), pair.second.begin(), pair.second.end());
            }
            // Merge timestamps
            combined.timestamps.insert(combined.timestamps.end(), 
                                       local.timestamps.begin(), local.timestamps.end());
        };
        
        LocalStatsData result = processFilesParallel(inputFiles, processFile, combineStats, LocalStatsData());
        
        // Copy results to member variables
        columnData = std::move(result.columnData);
        timestamps = std::move(result.timestamps);
    }
    
    printStats();
}

// ===== Usage printing =====

void StatsAnalyser::printStatsUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " stats [options] [<input_file(s)_or_directory(ies)>]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Calculate statistics for numeric sensor data." << std::endl;
    std::cerr << "Shows min, max, mean, median, and standard deviation for numeric columns." << std::endl;
    std::cerr << "If no input files are specified, reads from stdin (assumes JSON format unless -if is used)." << std::endl;
    std::cerr << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  -c, --column <name>       Analyze only this column (default: value, use 'all' for all columns)" << std::endl;
    std::cerr << "  -if, --input-format <fmt> Input format for stdin: json or csv (default: json)" << std::endl;
    std::cerr << "  -f, --follow              Follow mode: continuously read input and update stats (stdin or single file)" << std::endl;
    std::cerr << "  --only-value <col:val>    Only include rows where column has specific value (can be used multiple times)" << std::endl;
    std::cerr << "  --exclude-value <col:val> Exclude rows where column has specific value (can be used multiple times)" << std::endl;
    std::cerr << "  --not-empty <col>         Only include rows where column is not empty" << std::endl;
    std::cerr << "  --remove-empty-json       Remove rows with empty JSON objects" << std::endl;
    std::cerr << "  --remove-errors           Remove error readings (DS18B20 value=85 or -127)" << std::endl;
    std::cerr << "  --clean                   Shorthand for --remove-empty-json --not-empty value --remove-errors" << std::endl;
    std::cerr << "  -r, --recursive           Recursively process subdirectories" << std::endl;
    std::cerr << "  -v                        Verbose output" << std::endl;
    std::cerr << "  -V                        Very verbose output" << std::endl;
    std::cerr << "  -e, --extension <ext>     Filter files by extension (e.g., .out or out)" << std::endl;
    std::cerr << "  -d, --depth <n>           Maximum recursion depth (0 = current dir only)" << std::endl;
    std::cerr << "  --min-date <date>         Filter readings after this date (Unix timestamp, ISO date, or DD/MM/YYYY)" << std::endl;
    std::cerr << "  --max-date <date>         Filter readings before this date (Unix timestamp, ISO date, or DD/MM/YYYY)" << std::endl;
    std::cerr << "  --tail <n>                Only read the last n lines from each file" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Examples:" << std::endl;
    std::cerr << "  " << progName << " stats sensor1.out" << std::endl;
    std::cerr << "  " << progName << " stats < sensor1.out" << std::endl;
    std::cerr << "  " << progName << " stats -c value < sensor1.out" << std::endl;
    std::cerr << "  " << progName << " stats -c all < sensor1.out" << std::endl;
    std::cerr << "  " << progName << " stats -if csv < sensor1.csv" << std::endl;
    std::cerr << "  cat sensor1.out | " << progName << " stats" << std::endl;
    std::cerr << "  " << progName << " stats -r -e .out /path/to/logs/" << std::endl;
    std::cerr << "  " << progName << " stats sensor1.csv sensor2.out" << std::endl;
    std::cerr << "  " << progName << " stats --only-value sensor:ds18b20 sensor.out" << std::endl;
    std::cerr << "  tail -f sensor.out | " << progName << " stats --follow" << std::endl;
    std::cerr << "  " << progName << " stats --clean sensor.out  # exclude empty values" << std::endl;
}
