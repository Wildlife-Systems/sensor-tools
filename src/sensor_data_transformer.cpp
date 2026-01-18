#include "sensor_data_transformer.h"
#include <fstream>
#include <future>
#include <cstdio>
#include <array>

// ===== Private helper methods =====

bool SensorDataTransformer::hasActiveFilters() const {
    return !notEmptyColumns.empty() || !onlyValueFilters.empty() || 
           !excludeValueFilters.empty() || removeErrors || removeWhitespace || 
           removeEmptyJson || minDate > 0 || maxDate > 0;
}

bool SensorDataTransformer::getPrototypeColumns() {
    std::array<char, 128> buffer;
    std::string result;
    
    // Execute sc-prototype command
    FILE* pipe = popen("sc-prototype", "r");
    
    if (!pipe) {
        std::cerr << "Error: Failed to run sc-prototype command" << std::endl;
        return false;
    }
    
    // Read output
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    
    // Close pipe
    pclose(pipe);
    
    if (result.empty()) {
        std::cerr << "Error: sc-prototype returned no output" << std::endl;
        return false;
    }
    
    // Parse JSON to extract keys
    auto readings = JsonParser::parseJsonLine(result);
    if (readings.empty() || readings[0].empty()) {
        std::cerr << "Error: Failed to parse sc-prototype output" << std::endl;
        return false;
    }
    
    // Extract all keys from the prototype
    for (const auto& pair : readings[0]) {
        allKeys.insert(pair.first);
    }
    
    std::cout << "Loaded " << allKeys.size() << " columns from sc-prototype" << std::endl;
    return true;
}

void SensorDataTransformer::collectKeysFromFile(const std::string& filename) {
    if (verbosity >= 2) {
        std::cout << "Collecting keys from: " << filename << std::endl;
    }
    
    std::ifstream infile(filename);
    if (!infile) {
        std::cerr << "Warning: Cannot open file: " << filename << std::endl;
        return;
    }
    
    std::set<std::string> localKeys;
    std::string line;
    
    if (FileUtils::isCsvFile(filename)) {
        // CSV format - first line is header
        if (std::getline(infile, line) && !line.empty()) {
            auto fields = CsvParser::parseCsvLine(line);
            for (const auto& field : fields) {
                if (!field.empty()) {
                    localKeys.insert(field);
                }
            }
        }
    } else {
        // JSON format
        while (std::getline(infile, line)) {
            if (line.empty()) continue;
            
            auto readings = JsonParser::parseJsonLine(line);
            for (const auto& reading : readings) {
                for (const auto& pair : reading) {
                    localKeys.insert(pair.first);
                }
            }
        }
    }
    
    // Merge into global keys with mutex
    {
        std::lock_guard<std::mutex> lock(keysMutex);
        allKeys.insert(localKeys.begin(), localKeys.end());
        if (verbosity >= 2) {
            std::cout << "  Collected " << allKeys.size() << " unique keys so far" << std::endl;
        }
    }
    
    infile.close();
}

void SensorDataTransformer::writeRowsFromFile(const std::string& filename, std::ostream& outfile, 
                                               const std::vector<std::string>& headers) {
    if (verbosity >= 1) {
        std::cout << "Processing file: " << filename << std::endl;
    }
    
    std::ifstream infile(filename);
    if (!infile) {
        std::cerr << "Warning: Cannot open file: " << filename << std::endl;
        return;
    }
    
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
            
            if (!shouldIncludeReading(reading)) continue;
            writeRow(reading, headers, outfile);
        }
    } else {
        // JSON format
        while (std::getline(infile, line)) {
            if (line.empty()) continue;
            
            auto readings = JsonParser::parseJsonLine(line);
            for (const auto& reading : readings) {
                if (reading.empty()) continue;
                if (!shouldIncludeReading(reading)) continue;
                writeRow(reading, headers, outfile);
            }
        }
    }
    
    infile.close();
}

void SensorDataTransformer::writeRowsFromFileJson(const std::string& filename, std::ostream& outfile, 
                                                   bool& firstOutput) {
    if (verbosity >= 1) {
        std::cerr << "Processing file: " << filename << std::endl;
    }
    
    std::ifstream infile(filename);
    if (!infile) {
        std::cerr << "Warning: Cannot open file: " << filename << std::endl;
        return;
    }
    
    std::string line;
    const char* sp = removeWhitespace ? "" : " ";
    
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
            
            if (!shouldIncludeReading(reading)) continue;
            
            if (!firstOutput) outfile << "\n";
            firstOutput = false;
            outfile << "[" << sp;
            writeJsonObject(reading, outfile, removeWhitespace);
            outfile << sp << "]";
        }
    } else {
        // JSON format - preserve line-by-line structure
        while (std::getline(infile, line)) {
            if (line.empty()) continue;
            
            if (!hasActiveFilters()) {
                if (!firstOutput) outfile << "\n";
                firstOutput = false;
                outfile << line;
            } else {
                auto readings = JsonParser::parseJsonLine(line);
                std::vector<std::map<std::string, std::string>> filtered;
                
                for (const auto& reading : readings) {
                    if (reading.empty()) continue;
                    if (shouldIncludeReading(reading)) {
                        filtered.push_back(reading);
                    }
                }
                
                if (!filtered.empty()) {
                    if (!firstOutput) outfile << "\n";
                    firstOutput = false;
                    outfile << "[" << sp;
                    for (size_t i = 0; i < filtered.size(); ++i) {
                        if (i > 0) outfile << "," << sp;
                        writeJsonObject(filtered[i], outfile, removeWhitespace);
                    }
                    outfile << sp << "]";
                }
            }
        }
    }
    
    infile.close();
}

void SensorDataTransformer::processStdinData(const std::vector<std::string>& lines, 
                                              const std::vector<std::string>& headers, 
                                              std::ostream& outfile) {
    if (inputFormat == "csv") {
        std::vector<std::string> csvHeaders;
        bool isFirstLine = true;
        
        for (const auto& line : lines) {
            if (line.empty()) continue;
            
            if (isFirstLine) {
                csvHeaders = CsvParser::parseCsvLine(line);
                isFirstLine = false;
                continue;
            }
            
            auto fields = CsvParser::parseCsvLine(line);
            if (fields.empty()) continue;
            
            std::map<std::string, std::string> reading;
            for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                reading[csvHeaders[i]] = fields[i];
            }
            
            if (!shouldIncludeReading(reading)) continue;
            writeRow(reading, headers, outfile);
        }
    } else {
        for (const auto& line : lines) {
            if (line.empty()) continue;
            
            auto readings = JsonParser::parseJsonLine(line);
            for (const auto& reading : readings) {
                if (reading.empty()) continue;
                if (!shouldIncludeReading(reading)) continue;
                writeRow(reading, headers, outfile);
            }
        }
    }
}

void SensorDataTransformer::processStdinDataJson(const std::vector<std::string>& lines, 
                                                  std::ostream& outfile) {
    bool firstOutput = true;
    const char* sp = removeWhitespace ? "" : " ";
    
    if (inputFormat == "csv") {
        std::vector<std::string> csvHeaders;
        bool isFirstLine = true;
        
        for (const auto& line : lines) {
            if (line.empty()) continue;
            
            if (isFirstLine) {
                csvHeaders = CsvParser::parseCsvLine(line);
                isFirstLine = false;
                continue;
            }
            
            auto fields = CsvParser::parseCsvLine(line);
            if (fields.empty()) continue;
            
            std::map<std::string, std::string> reading;
            for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                reading[csvHeaders[i]] = fields[i];
            }
            
            if (!shouldIncludeReading(reading)) continue;
            
            if (!firstOutput) outfile << "\n";
            firstOutput = false;
            outfile << "[" << sp;
            writeJsonObject(reading, outfile, removeWhitespace);
            outfile << sp << "]";
        }
    }
}

void SensorDataTransformer::writeRow(const std::map<std::string, std::string>& reading,
                                      const std::vector<std::string>& headers,
                                      std::ostream& outfile) {
    if (outputFormat == "json") {
        writeJsonObject(reading, outfile, removeWhitespace);
        outfile << "\n";
    } else {
        writeCsvRow(reading, headers, outfile);
    }
}

// ===== Constructor =====

SensorDataTransformer::SensorDataTransformer(int argc, char* argv[]) 
    : outputFormat("")
    , removeWhitespace(false)
    , numThreads(4)
    , usePrototype(false) {
    
    // Check for help flag first
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printTransformUsage(argv[0]);
            exit(0);
        }
    }
    
    outputFile = "";
    
    // First pass: parse transformer-specific flags
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--use-prototype") {
            usePrototype = true;
        } else if (arg == "--remove-whitespace") {
            removeWhitespace = true;
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                ++i;
                outputFile = argv[i];
            } else {
                std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                exit(1);
            }
        } else if (arg == "-of" || arg == "--output-format") {
            if (i + 1 < argc) {
                ++i;
                outputFormat = argv[i];
                if (outputFormat != "json" && outputFormat != "csv") {
                    std::cerr << "Error: --output-format must be 'json' or 'csv'" << std::endl;
                    exit(1);
                }
            } else {
                std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                exit(1);
            }
        } else {
            // Try to parse as filter option
            int result = parseFilterOption(argc, argv, i, arg);
            if (result >= 0) {
                i = result;
            }
            // Ignore unrecognized options here - CommonArgParser handles them
        }
    }
    
    // Second pass: parse common flags and collect files
    CommonArgParser commonParser;
    if (!commonParser.parse(argc, argv)) {
        exit(1);
    }
    
    copyFromParser(commonParser);
}

// ===== Main transform method =====

void SensorDataTransformer::transform() {
    if (outputFormat.empty()) {
        outputFormat = "json";
    }
    
    if (inputFiles.empty()) {
        // Reading from stdin
        if (verbosity >= 1) {
            std::cerr << "Reading from stdin (format: " << inputFormat << ")..." << std::endl;
        }
        printFilterInfo();
        
        // Streaming path: JSON input -> JSON output
        if (inputFormat == "json" && outputFormat == "json") {
            std::ostream* outStream = &std::cout;
            std::ofstream outFile;
            
            if (!outputFile.empty()) {
                outFile.open(outputFile);
                if (!outFile) {
                    std::cerr << "Error: Cannot create output file: " << outputFile << std::endl;
                    return;
                }
                outStream = &outFile;
            }
            
            std::string line;
            bool firstOutput = true;
            bool hasInput = false;
            const char* sp = removeWhitespace ? "" : " ";
            
            while (std::getline(std::cin, line)) {
                if (line.empty()) continue;
                hasInput = true;
                
                if (!hasActiveFilters()) {
                    if (!firstOutput) *outStream << "\n";
                    firstOutput = false;
                    *outStream << line;
                } else {
                    auto readings = JsonParser::parseJsonLine(line);
                    std::vector<std::map<std::string, std::string>> filtered;
                    
                    for (const auto& reading : readings) {
                        if (reading.empty()) continue;
                        if (shouldIncludeReading(reading)) {
                            filtered.push_back(reading);
                        }
                    }
                    
                    if (!filtered.empty()) {
                        if (!firstOutput) *outStream << "\n";
                        firstOutput = false;
                        *outStream << "[" << sp;
                        for (size_t i = 0; i < filtered.size(); ++i) {
                            if (i > 0) *outStream << "," << sp;
                            writeJsonObject(filtered[i], *outStream, removeWhitespace);
                        }
                        *outStream << sp << "]";
                    }
                }
                
                if (outputFile.empty()) {
                    outStream->flush();
                }
            }
            
            if (!hasInput) {
                std::cerr << "Error: No input data" << std::endl;
                if (!outputFile.empty()) {
                    outFile.close();
                }
                return;
            }
            
            *outStream << "\n";
            if (!outputFile.empty()) {
                outFile.close();
                if (verbosity >= 1) {
                    std::cerr << "Wrote json to " << outputFile << std::endl;
                }
            }
            return;
        }
        
        // Buffered path: needed for CSV output
        std::vector<std::string> stdinLines;
        std::string line;
        std::set<std::string> stdinKeys;
        
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;
            stdinLines.push_back(line);
            
            if (inputFormat == "csv") {
                if (stdinKeys.empty()) {
                    auto fields = CsvParser::parseCsvLine(line);
                    for (const auto& field : fields) {
                        if (!field.empty()) {
                            stdinKeys.insert(field);
                        }
                    }
                }
            } else {
                auto readings = JsonParser::parseJsonLine(line);
                for (const auto& reading : readings) {
                    for (const auto& pair : reading) {
                        stdinKeys.insert(pair.first);
                    }
                }
            }
        }
        
        if (stdinLines.empty()) {
            std::cerr << "Error: No input data" << std::endl;
            return;
        }
        
        allKeys = stdinKeys;
        std::vector<std::string> headers(allKeys.begin(), allKeys.end());
        std::sort(headers.begin(), headers.end());
        
        if (outputFile.empty()) {
            if (outputFormat == "json") {
                processStdinDataJson(stdinLines, std::cout);
                std::cout << "\n";
            } else {
                for (size_t i = 0; i < headers.size(); ++i) {
                    if (i > 0) std::cout << ",";
                    std::cout << headers[i];
                }
                std::cout << "\n";
                processStdinData(stdinLines, headers, std::cout);
            }
        } else {
            std::ofstream outfile(outputFile);
            if (!outfile) {
                std::cerr << "Error: Cannot create output file: " << outputFile << std::endl;
                return;
            }
            
            if (outputFormat == "json") {
                processStdinDataJson(stdinLines, outfile);
                outfile << "\n";
            } else {
                for (size_t i = 0; i < headers.size(); ++i) {
                    if (i > 0) outfile << ",";
                    outfile << headers[i];
                }
                outfile << "\n";
                processStdinData(stdinLines, headers, outfile);
            }
            
            outfile.close();
            if (verbosity >= 1) {
                std::cerr << "Wrote " << outputFormat << " to " << outputFile << std::endl;
            }
        }
        return;
    }
    
    // File-based processing
    printCommonVerboseInfo("Starting conversion", verbosity, recursive, extensionFilter, maxDepth, inputFiles.size());
    printFilterInfo();
    
    // PASS 1: Collect all column names
    if (usePrototype) {
        std::cerr << "Using sc-prototype for column definitions..." << std::endl;
        if (!getPrototypeColumns()) {
            std::cerr << "Error: Failed to get prototype columns" << std::endl;
            return;
        }
    } else {
        std::cerr << "Pass 1: Discovering columns..." << std::endl;
        
        size_t filesPerThread = std::max(size_t(1), inputFiles.size() / numThreads);
        std::vector<std::future<void>> futures;
        futures.reserve(numThreads);
        
        for (size_t i = 0; i < inputFiles.size(); i += filesPerThread) {
            size_t end = std::min(i + filesPerThread, inputFiles.size());
            
            futures.push_back(std::async(std::launch::async, [this, i, end]() {
                for (size_t j = i; j < end; ++j) {
                    collectKeysFromFile(inputFiles[j]);
                }
            }));
        }
        
        for (auto& f : futures) {
            f.wait();
        }
        
        std::cerr << "Found " << allKeys.size() << " unique fields" << std::endl;
    }
    
    std::vector<std::string> headers(allKeys.begin(), allKeys.end());
    std::sort(headers.begin(), headers.end());
    
    // PASS 2: Stream data to output
    if (outputFile.empty()) {
        if (verbosity >= 1) {
            std::cerr << "Pass 2: Writing " << outputFormat << " to stdout..." << std::endl;
        }
        
        if (outputFormat == "json") {
            bool firstOutput = true;
            for (const auto& file : inputFiles) {
                writeRowsFromFileJson(file, std::cout, firstOutput);
            }
            std::cout << "\n";
        } else {
            for (size_t i = 0; i < headers.size(); ++i) {
                if (i > 0) std::cout << ",";
                std::cout << headers[i];
            }
            std::cout << "\n";
            
            for (const auto& file : inputFiles) {
                writeRowsFromFile(file, std::cout, headers);
            }
        }
    } else {
        if (verbosity >= 1) {
            std::cerr << "Pass 2: Writing " << outputFormat << " to file..." << std::endl;
        }
        
        std::ofstream outfile(outputFile);
        if (!outfile) {
            std::cerr << "Error: Cannot create output file: " << outputFile << std::endl;
            return;
        }
        
        if (outputFormat == "json") {
            bool firstOutput = true;
            for (const auto& file : inputFiles) {
                writeRowsFromFileJson(file, outfile, firstOutput);
            }
            outfile << "\n";
        } else {
            for (size_t i = 0; i < headers.size(); ++i) {
                if (i > 0) outfile << ",";
                outfile << headers[i];
            }
            outfile << "\n";
            
            for (const auto& file : inputFiles) {
                writeRowsFromFile(file, outfile, headers);
            }
        }
        
        outfile.close();
        if (verbosity >= 1) {
            std::cerr << "Wrote " << outputFormat << " to " << outputFile << std::endl;
        }
    }
}

// ===== Usage printing =====

void SensorDataTransformer::printTransformUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " transform [options] [<input_file(s)_or_directory(ies)>]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Transform JSON or CSV sensor data files to JSON or CSV format." << std::endl;
    std::cerr << "For JSON: Each line in input files should contain JSON with sensor readings." << std::endl;
    std::cerr << "For CSV: Files with .csv extension are automatically detected and processed." << std::endl;
    std::cerr << "Each sensor reading will become a row in the output." << std::endl;
    std::cerr << "If no input files are specified, reads from stdin (assumes JSON format unless -if is used)." << std::endl;
    std::cerr << "Output is written to stdout unless -o/--output is specified." << std::endl;
    std::cerr << "Default output format: JSON (matching .out file format)." << std::endl;
    std::cerr << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  -o, --output <file>       Output file (default: stdout)" << std::endl;
    std::cerr << "  -if, --input-format <fmt>  Input format for stdin: json or csv (default: json)" << std::endl;
    std::cerr << "  -of, --output-format <fmt> Output format: json or csv (default: json)" << std::endl;
    std::cerr << "  -r, --recursive           Recursively process subdirectories" << std::endl;
    std::cerr << "  -v                        Verbose output (show progress)" << std::endl;
    std::cerr << "  -V                        Very verbose output (show detailed progress)" << std::endl;
    std::cerr << "  -e, --extension <ext>     Filter files by extension (e.g., .out or out)" << std::endl;
    std::cerr << "  -d, --depth <n>           Maximum recursion depth (0 = current dir only)" << std::endl;
    std::cerr << "  --use-prototype           Use sc-prototype command to define columns" << std::endl;
    std::cerr << "  --not-empty <column>      Skip rows where column is empty (can be used multiple times)" << std::endl;
    std::cerr << "  --only-value <col:val>    Only include rows where column has specific value (can be used multiple times)" << std::endl;
    std::cerr << "  --exclude-value <col:val> Exclude rows where column has specific value (can be used multiple times)" << std::endl;
    std::cerr << "  --remove-errors           Remove error readings (DS18B20 value=85 or -127)" << std::endl;
    std::cerr << "  --remove-whitespace       Remove extra whitespace from output (compact format)" << std::endl;
    std::cerr << "  --remove-empty-json       Remove empty JSON input lines (e.g., [{}], [])" << std::endl;
    std::cerr << "  --min-date <date>         Filter readings after this date (Unix timestamp, ISO date, or DD/MM/YYYY)" << std::endl;
    std::cerr << "  --max-date <date>         Filter readings before this date (Unix timestamp, ISO date, or DD/MM/YYYY)" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Examples:" << std::endl;
    std::cerr << "  " << progName << " transform sensor1.out" << std::endl;
    std::cerr << "  " << progName << " transform < sensor1.out" << std::endl;
    std::cerr << "  " << progName << " transform -if csv < sensor1.csv" << std::endl;
    std::cerr << "  cat sensor1.out | " << progName << " transform" << std::endl;
    std::cerr << "  cat sensor1.out | " << progName << " transform -o output.csv" << std::endl;
    std::cerr << "  " << progName << " transform -o output.csv sensor1.out" << std::endl;
    std::cerr << "  " << progName << " transform -o output.csv sensor1.csv sensor2.csv" << std::endl;
    std::cerr << "  " << progName << " transform -o output.csv sensor1.out sensor2.out" << std::endl;
    std::cerr << "  " << progName << " transform --remove-errors -o output.csv sensor1.out" << std::endl;
    std::cerr << "  " << progName << " transform -e .out -o output.csv /path/to/sensor/dir" << std::endl;
    std::cerr << "  " << progName << " transform -r -e .csv -o output.csv /path/to/sensor/dir" << std::endl;
    std::cerr << "  " << progName << " transform -r -e .out -o output.csv /path/to/sensor/dir" << std::endl;
    std::cerr << "  " << progName << " transform -r -d 2 -e .out -o output.csv /path/to/logs" << std::endl;
    std::cerr << "  " << progName << " transform --use-prototype -r -e .out -o output.csv /path/to/logs" << std::endl;
    std::cerr << "  " << progName << " transform --not-empty unit --not-empty value -e .out -o output.csv /logs" << std::endl;
    std::cerr << "  " << progName << " transform --only-value type:temperature -r -e .out -o output.csv /logs" << std::endl;
    std::cerr << "  " << progName << " transform --only-value type:temperature --only-value unit:C -o output.csv /logs" << std::endl;
}
