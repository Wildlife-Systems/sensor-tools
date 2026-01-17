#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cstring>
#include <sys/stat.h>
#include <thread>
#include <mutex>
#include <future>
#include <memory>
#include <cstdio>
#include <array>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

// CSV parser for sensor data
class CsvParser {
public:
    // Parse CSV line considering proper escaping and quoting
    // Note: This parser handles multi-line fields by reading additional lines when needed
    static std::vector<std::string> parseCsvLine(std::istream& input, std::string& line, bool& needMoreLines) {
        std::vector<std::string> fields;
        std::string current;
        bool inQuotes = false;
        needMoreLines = false;
        
        while (true) {
            for (size_t i = 0; i < line.length(); ++i) {
                char c = line[i];
                
                if (inQuotes) {
                    if (c == '"') {
                        // Check if it's an escaped quote
                        if (i + 1 < line.length() && line[i + 1] == '"') {
                            current += '"';
                            ++i;  // Skip next quote
                        } else {
                            inQuotes = false;
                        }
                    } else {
                        current += c;
                    }
                } else {
                    if (c == '"') {
                        inQuotes = true;
                    } else if (c == ',') {
                        fields.push_back(current);
                        current.clear();
                    } else if (c != '\r') {  // Ignore CR in CRLF
                        current += c;
                    }
                }
            }
            
            // If we're still in quotes, we need to read more lines
            if (inQuotes) {
                current += '\n';  // Add newline that was consumed by getline
                if (std::getline(input, line)) {
                    needMoreLines = true;
                    continue;  // Continue parsing with the next line
                } else {
                    // End of file while in quotes - malformed CSV
                    break;
                }
            } else {
                break;  // Done parsing this logical line
            }
        }
        
        fields.push_back(current);
        needMoreLines = false;
        return fields;
    }
    
    // Simpler version for single-line parsing (backwards compatibility)
    static std::vector<std::string> parseCsvLine(const std::string& line) {
        std::vector<std::string> fields;
        std::string current;
        bool inQuotes = false;
        
        for (size_t i = 0; i < line.length(); ++i) {
            char c = line[i];
            
            if (inQuotes) {
                if (c == '"') {
                    // Check if it's an escaped quote
                    if (i + 1 < line.length() && line[i + 1] == '"') {
                        current += '"';
                        ++i;  // Skip next quote
                    } else {
                        inQuotes = false;
                    }
                } else {
                    current += c;
                }
            } else {
                if (c == '"') {
                    inQuotes = true;
                } else if (c == ',') {
                    fields.push_back(current);
                    current.clear();
                } else if (c != '\r') {  // Ignore CR in CRLF
                    current += c;
                }
            }
        }
        
        fields.push_back(current);
        return fields;
    }
};

// Simple JSON parser for sensor data
class JsonParser {
public:
    static std::vector<std::map<std::string, std::string>> parseJsonLine(const std::string& line) {
        std::vector<std::map<std::string, std::string>> readings;
        
        // Find the main object or array
        size_t start = line.find('{');
        if (start == std::string::npos) {
            start = line.find('[');
        }
        
        if (start == std::string::npos) {
            return readings;
        }
        
        // Simple JSON parsing - looks for key-value pairs
        std::map<std::string, std::string> current;
        size_t pos = start;
        
        while (pos < line.length()) {
            // Find key
            size_t keyStart = line.find('"', pos);
            if (keyStart == std::string::npos) break;
            
            size_t keyEnd = line.find('"', keyStart + 1);
            if (keyEnd == std::string::npos) break;
            
            std::string key = line.substr(keyStart + 1, keyEnd - keyStart - 1);
            
            // Find colon
            size_t colon = line.find(':', keyEnd);
            if (colon == std::string::npos) break;
            
            // Find value
            size_t valueStart = colon + 1;
            while (valueStart < line.length() && isspace(line[valueStart])) valueStart++;
            
            std::string value;
            if (line[valueStart] == '"') {
                // String value
                size_t valueEnd = line.find('"', valueStart + 1);
                if (valueEnd == std::string::npos) break;
                // Reserve space to reduce allocations
                value.reserve(valueEnd - valueStart - 1);
                value = line.substr(valueStart + 1, valueEnd - valueStart - 1);
                pos = valueEnd + 1;
            } else if (line[valueStart] == '[') {
                // Array - this might contain sensor readings
                size_t bracketEnd = line.find(']', valueStart);
                if (bracketEnd == std::string::npos) break;
                
                // If we have accumulated data and hit an array, might be multiple readings
                if (!current.empty() && key == "readings") {
                    // Parse array of readings
                    value.reserve(bracketEnd - valueStart - 1);
                    value = line.substr(valueStart + 1, bracketEnd - valueStart - 1);
                }
                pos = bracketEnd + 1;
            } else {
                // Numeric or other value
                size_t valueEnd = line.find_first_of(",}]", valueStart);
                if (valueEnd == std::string::npos) valueEnd = line.length();
                value = line.substr(valueStart, valueEnd - valueStart);
                // Trim whitespace - optimize by finding actual end first
                size_t end = value.find_last_not_of(" \t\n\r");
                if (end != std::string::npos) value = value.substr(0, end + 1);
                pos = valueEnd;
            }
            
            current[key] = value;
            
            // Check if we hit end of object
            if (pos < line.length() && line[pos] == '}') {
                if (!current.empty()) {
                    readings.push_back(current);
                    current.clear();
                }
                pos++;
            }
        }
        
        if (!current.empty()) {
            readings.push_back(current);
        }
        
        // If no readings found, return empty
        if (readings.empty()) {
            readings.push_back(current);
        }
        
        return readings;
    }
};

class SensorDataConverter {
private:
    std::vector<std::string> inputFiles;
    std::string outputFile;
    std::set<std::string> allKeys;
    std::mutex keysMutex;  // For thread-safe key collection
    bool hasInputFiles;
    bool recursive;
    std::string extensionFilter;
    int maxDepth;
    int numThreads;
    bool usePrototype;
    std::set<std::string> notEmptyColumns;  // Columns that must not be empty
    std::map<std::string, std::string> onlyValueFilters;  // Column:value pairs for filtering
    int verbosity;  // 0 = normal, 1 = verbose (-v), 2 = very verbose (-V)
    bool removeErrors;  // Remove error readings (e.g., DS18B20 temperature = 85)
    
    // Helper to detect if a reading contains an error
    bool isErrorReading(const std::map<std::string, std::string>& reading) {
        // Check for DS18B20 sensor error (temperature = 85)
        auto typeIt = reading.find("type");
        auto valueIt = reading.find("value");
        auto tempIt = reading.find("temperature");
        
        // Check if type is ds18b20 or DS18B20 (case insensitive check)
        bool isDS18B20 = false;
        if (typeIt != reading.end()) {
            std::string type = typeIt->second;
            std::transform(type.begin(), type.end(), type.begin(), ::tolower);
            isDS18B20 = (type == "ds18b20");
        }
        
        if (isDS18B20) {
            // Check value field
            if (valueIt != reading.end() && valueIt->second == "85") {
                return true;
            }
            // Also check temperature field as fallback
            if (tempIt != reading.end() && tempIt->second == "85") {
                return true;
            }
        }
        
        return false;
    }
    
    // Helper to detect if a file is CSV based on extension
    bool isCsvFile(const std::string& filename) {
        size_t dotPos = filename.find_last_of('.');
        if (dotPos == std::string::npos) {
            return false;
        }
        std::string ext = filename.substr(dotPos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".csv";
    }
    
    bool isDirectory(const std::string& path) {
        struct stat info;
        if (stat(path.c_str(), &info) != 0) {
            return false;
        }
        return (info.st_mode & S_IFDIR) != 0;
    }
    
    bool matchesExtension(const std::string& filename) {
        if (extensionFilter.empty()) {
            return true;
        }
        size_t dotPos = filename.find_last_of('.');
        if (dotPos == std::string::npos) {
            return false;
        }
        std::string ext = filename.substr(dotPos);
        return ext == extensionFilter;
    }
    
    // Execute sc-prototype command and parse columns from JSON output
    bool getPrototypeColumns() {
        std::array<char, 128> buffer;
        std::string result;
        
        // Execute sc-prototype command
        FILE* pipe = nullptr;
#ifdef _WIN32
        pipe = _popen("sc-prototype", "r");
#else
        pipe = popen("sc-prototype", "r");
#endif
        
        if (!pipe) {
            std::cerr << "Error: Failed to run sc-prototype command" << std::endl;
            return false;
        }
        
        // Read output
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
        
        // Close pipe
#ifdef _WIN32
        _pclose(pipe);
#else
        pclose(pipe);
#endif
        
        if (result.empty()) {
            std::cerr << "Error: sc-prototype returned no output" << std::endl;
            return false;
        }
        
        // Parse JSON to extract keys
        // The output is a single JSON object with all fields set to null
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
    
    void collectFilesFromDirectory(const std::string& dirPath, int currentDepth = 0) {
        // Check depth limit
        if (maxDepth >= 0 && currentDepth > maxDepth) {
            if (verbosity >= 2) {
                std::cout << "Skipping directory (depth limit): " << dirPath << std::endl;
            }
            return;
        }
        
        if (verbosity >= 1) {
            std::cout << "Scanning directory: " << dirPath << " (depth " << currentDepth << ")" << std::endl;
        }
        
#ifdef _WIN32
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA((dirPath + "\\*").c_str(), &findData);
        
        if (hFind == INVALID_HANDLE_VALUE) {
            std::cerr << "Warning: Cannot open directory: " << dirPath << std::endl;
            return;
        }
        
        do {
            std::string filename = findData.cFileName;
            if (filename != "." && filename != "..") {
                std::string fullPath = dirPath + "\\" + filename;
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (recursive) {
                        collectFilesFromDirectory(fullPath, currentDepth + 1);
                    }
                } else {
                    if (matchesExtension(filename)) {
                        if (verbosity >= 2) {
                            std::cout << "  Found file: " << fullPath << std::endl;
                        }
                        inputFiles.push_back(fullPath);
                    } else if (verbosity >= 2 && !extensionFilter.empty()) {
                        std::cout << "  Skipping (extension): " << fullPath << std::endl;
                    }
                }
            }
        } while (FindNextFileA(hFind, &findData) != 0);
        
        FindClose(hFind);
#else
        DIR* dir = opendir(dirPath.c_str());
        if (!dir) {
            std::cerr << "Warning: Cannot open directory: " << dirPath << std::endl;
            return;
        }
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string filename = entry->d_name;
            if (filename != "." && filename != "..") {
                std::string fullPath = dirPath + "/" + filename;
                if (isDirectory(fullPath)) {
                    if (recursive) {
                        collectFilesFromDirectory(fullPath, currentDepth + 1);
                    }
                } else {
                    if (matchesExtension(filename)) {
                        if (verbosity >= 2) {
                            std::cout << "  Found file: " << fullPath << std::endl;
                        }
                        inputFiles.push_back(fullPath);
                    } else if (verbosity >= 2 && !extensionFilter.empty()) {
                        std::cout << "  Skipping (extension): " << fullPath << std::endl;
                    }
                }
            }
        }
        closedir(dir);
#endif
    }
    
    // First pass: collect all column names from a file (thread-safe)
    void collectKeysFromFile(const std::string& filename) {
        if (verbosity >= 2) {
            std::cout << "Collecting keys from: " << filename << std::endl;
        }
        
        std::ifstream infile(filename);
        if (!infile) {
            std::cerr << "Warning: Cannot open file: " << filename << std::endl;
            return;
        }
        
        std::set<std::string> localKeys;  // Thread-local key collection
        std::string line;
        
        // Check if this is a CSV file
        if (isCsvFile(filename)) {
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
    
    // Second pass: write rows from a file directly to CSV (not thread-safe, called sequentially)
    void writeRowsFromFile(const std::string& filename, std::ofstream& outfile, 
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
        
        // Check if this is a CSV file
        if (isCsvFile(filename)) {
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
                
                // Create a map from CSV headers to values
                std::map<std::string, std::string> reading;
                for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                    reading[csvHeaders[i]] = fields[i];
                }
                
                // Apply same filtering logic as JSON
                bool skipRow = false;
                std::string skipReason;
                
                // Check if any required columns are empty
                for (const auto& reqCol : notEmptyColumns) {
                    auto it = reading.find(reqCol);
                    if (it == reading.end() || it->second.empty()) {
                        skipRow = true;
                        if (verbosity >= 2) {
                            skipReason = it == reading.end() ? "missing column '" + reqCol + "'" : "empty column '" + reqCol + "'";
                        }
                        break;
                    }
                }
                
                if (skipRow) {
                    if (verbosity >= 2) {
                        std::cout << "  Skipping row: " << skipReason << std::endl;
                    }
                    continue;
                }
                
                // Check value filters
                for (const auto& filter : onlyValueFilters) {
                    auto it = reading.find(filter.first);
                    if (it == reading.end() || it->second != filter.second) {
                        skipRow = true;
                        if (verbosity >= 2) {
                            if (it == reading.end()) {
                                skipReason = "missing column '" + filter.first + "'";
                            } else {
                                skipReason = "column '" + filter.first + "' has value '" + it->second + "' (expected '" + filter.second + "')";
                            }
                        }
                        break;
                    }
                }
                
                if (skipRow) {
                    if (verbosity >= 2) {
                        std::cout << "  Skipping row: " << skipReason << std::endl;
                    }
                    continue;
                }
                
                // Check for error readings
                if (removeErrors && isErrorReading(reading)) {
                    if (verbosity >= 2) {
                        std::cout << "  Skipping error reading (DS18B20 temperature=85)" << std::endl;
                    }
                    continue;
                }
                
                // Write row with unified headers
                for (size_t i = 0; i < headers.size(); ++i) {
                    if (i > 0) outfile << ",";
                    
                    auto it = reading.find(headers[i]);
                    if (it != reading.end()) {
                        std::string value = it->second;
                        // Escape quotes and wrap in quotes if contains comma or quote
                        if (value.find(',') != std::string::npos || 
                            value.find('"') != std::string::npos ||
                            value.find('\n') != std::string::npos) {
                            // Escape quotes
                            size_t pos = 0;
                            while ((pos = value.find('"', pos)) != std::string::npos) {
                                value.replace(pos, 1, "\"\"");
                                pos += 2;
                            }
                            outfile << "\"" << value << "\"";
                        } else {
                            outfile << value;
                        }
                    }
                }
                outfile << "\n";
            }
        } else {
            // JSON format (original code)
            while (std::getline(infile, line)) {
                if (line.empty()) continue;
                
                auto readings = JsonParser::parseJsonLine(line);
                for (const auto& reading : readings) {
                    if (reading.empty()) continue;
                    
                    // Check if any required columns are empty
                    bool skipRow = false;
                    std::string skipReason;
                    for (const auto& reqCol : notEmptyColumns) {
                        auto it = reading.find(reqCol);
                        if (it == reading.end() || it->second.empty()) {
                            skipRow = true;
                            if (verbosity >= 2) {
                                skipReason = it == reading.end() ? "missing column '" + reqCol + "'" : "empty column '" + reqCol + "'";
                            }
                            break;
                        }
                    }
                    
                    if (skipRow) {
                        if (verbosity >= 2) {
                            std::cout << "  Skipping row: " << skipReason << std::endl;
                        }
                        continue;
                    }
                    
                    // Check value filters
                    for (const auto& filter : onlyValueFilters) {
                        auto it = reading.find(filter.first);
                        if (it == reading.end() || it->second != filter.second) {
                            skipRow = true;
                            if (verbosity >= 2) {
                                if (it == reading.end()) {
                                    skipReason = "missing column '" + filter.first + "'";
                                } else {
                                    skipReason = "column '" + filter.first + "' has value '" + it->second + "' (expected '" + filter.second + "')";
                                }
                            }
                            break;
                        }
                    }
                    
                    if (skipRow) {
                        if (verbosity >= 2) {
                            std::cout << "  Skipping row: " << skipReason << std::endl;
                        }
                        continue;
                    }
                    
                    // Check for error readings
                    if (removeErrors && isErrorReading(reading)) {
                        if (verbosity >= 2) {
                            std::cout << "  Skipping error reading (DS18B20 temperature=85)" << std::endl;
                        }
                        continue;
                    }
                    
                    // Write row
                    for (size_t i = 0; i < headers.size(); ++i) {
                        if (i > 0) outfile << ",";
                        
                        auto it = reading.find(headers[i]);
                        if (it != reading.end()) {
                            std::string value = it->second;
                            // Escape quotes and wrap in quotes if contains comma or quote
                            if (value.find(',') != std::string::npos || 
                                value.find('"') != std::string::npos ||
                                value.find('\n') != std::string::npos) {
                                // Escape quotes
                                size_t pos = 0;
                                while ((pos = value.find('"', pos)) != std::string::npos) {
                                    value.replace(pos, 1, "\"\"");
                                    pos += 2;
                                }
                                outfile << "\"" << value << "\"";
                            } else {
                                outfile << value;
                            }
                        }
                    }
                    outfile << "\n";
                }
            }
        }
        
        if (verbosity >= 2) {
            std::lock_guard<std::mutex> lock(keysMutex);
            std::cout << "  Collected " << allKeys.size() << " unique keys so far" << std::endl;
        }
        
        infile.close();
    }
    
public:
    SensorDataConverter(int argc, char* argv[]) : hasInputFiles(false), recursive(false), extensionFilter(""), maxDepth(-1), numThreads(4), usePrototype(false), verbosity(0), removeErrors(false) {
        // argc should be at least 3 for "convert": program convert input output
        if (argc < 3) {
            printConvertUsage(argv[0]);
            exit(1);
        }
        
        // Last argument is output file
        outputFile = argv[argc - 1];
        
        // Reserve some capacity for input files
        inputFiles.reserve(100);
        
        // Parse flags and arguments from index 1 to argc-2
        for (int i = 1; i < argc - 1; ++i) {
            std::string arg = argv[i];
            
            if (arg == "-r" || arg == "--recursive") {
                recursive = true;
            } else if (arg == "-v") {
                verbosity = 1;  // verbose
            } else if (arg == "-V") {
                verbosity = 2;  // very verbose
            } else if (arg == "--use-prototype") {
                usePrototype = true;
            } else if (arg == "--remove-errors") {
                removeErrors = true;
            } else if (arg == "-e" || arg == "--extension") {
                if (i + 1 < argc - 1) {
                    ++i;
                    extensionFilter = argv[i];
                    // Ensure extension starts with a dot
                    if (!extensionFilter.empty() && extensionFilter[0] != '.') {
                        extensionFilter = "." + extensionFilter;
                    }
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    exit(1);
                }
            } else if (arg == "-d" || arg == "--depth") {
                if (i + 1 < argc - 1) {
                    ++i;
                    try {
                        maxDepth = std::stoi(argv[i]);
                        if (maxDepth < 0) {
                            std::cerr << "Error: depth must be non-negative" << std::endl;
                            exit(1);
                        }
                    } catch (...) {
                        std::cerr << "Error: invalid depth value '" << argv[i] << "'" << std::endl;
                        exit(1);
                    }
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    exit(1);
                }
            } else if (arg == "--not-empty") {
                if (i + 1 < argc - 1) {
                    ++i;
                    notEmptyColumns.insert(argv[i]);
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    exit(1);
                }
            } else if (arg == "--only-value") {
                if (i + 1 < argc - 1) {
                    ++i;
                    std::string filter = argv[i];
                    size_t colonPos = filter.find(':');
                    if (colonPos == std::string::npos || colonPos == 0 || colonPos == filter.length() - 1) {
                        std::cerr << "Error: --only-value requires format 'column:value'" << std::endl;
                        exit(1);
                    }
                    std::string column = filter.substr(0, colonPos);
                    std::string value = filter.substr(colonPos + 1);
                    onlyValueFilters[column] = value;
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    exit(1);
                }
            } else if (arg[0] == '-') {
                std::cerr << "Error: Unknown option '" << arg << "'" << std::endl;
                printConvertUsage(argv[0]);
                exit(1);
            } else {
                // It's a file or directory path
                if (isDirectory(arg)) {
                    collectFilesFromDirectory(arg);
                } else {
                    inputFiles.push_back(arg);
                }
            }
        }
        
        if (inputFiles.empty()) {
            std::cerr << "Error: No input files found" << std::endl;
            exit(1);
        }
        
        hasInputFiles = true;
    }
    
    void convert() {
        if (inputFiles.empty()) {
            std::cerr << "Error: No input files to process" << std::endl;
            return;
        }
        
        if (verbosity >= 1) {
            std::cout << "Starting conversion with verbosity level " << verbosity << std::endl;
            std::cout << "Recursive: " << (recursive ? "yes" : "no") << std::endl;
            if (!extensionFilter.empty()) {
                std::cout << "Extension filter: " << extensionFilter << std::endl;
            }
            if (maxDepth >= 0) {
                std::cout << "Max depth: " << maxDepth << std::endl;
            }
            if (!notEmptyColumns.empty()) {
                std::cout << "Required non-empty columns: ";
                bool first = true;
                for (const auto& col : notEmptyColumns) {
                    if (!first) std::cout << ", ";
                    std::cout << col;
                    first = false;
                }
                std::cout << std::endl;
            }
            if (!onlyValueFilters.empty()) {
                std::cout << "Value filters: ";
                bool first = true;
                for (const auto& filter : onlyValueFilters) {
                    if (!first) std::cout << ", ";
                    std::cout << filter.first << "=" << filter.second;
                    first = false;
                }
                std::cout << std::endl;
            }
        }
        
        std::cout << "Processing " << inputFiles.size() << " file(s)..." << std::endl;
        
        // PASS 1: Collect all column names (skip if using prototype)
        if (usePrototype) {
            std::cout << "Using sc-prototype for column definitions..." << std::endl;
            if (!getPrototypeColumns()) {
                std::cerr << "Error: Failed to get prototype columns" << std::endl;
                return;
            }
        } else {
            std::cout << "Pass 1: Discovering columns..." << std::endl;
            
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
            
            // Wait for all threads to complete
            for (auto& f : futures) {
                f.wait();
            }
            
            std::cout << "Found " << allKeys.size() << " unique fields" << std::endl;
        }
        
        // Convert set to vector for consistent ordering
        std::vector<std::string> headers(allKeys.begin(), allKeys.end());
        std::sort(headers.begin(), headers.end());
        
        // PASS 2: Stream data to CSV
        std::cout << "Pass 2: Writing CSV..." << std::endl;
        
        std::ofstream outfile(outputFile);
        if (!outfile) {
            std::cerr << "Error: Cannot create output file: " << outputFile << std::endl;
            return;
        }
        
        // Write header
        for (size_t i = 0; i < headers.size(); ++i) {
            if (i > 0) outfile << ",";
            outfile << headers[i];
        }
        outfile << "\n";
        
        // Write rows from each file sequentially (streaming)
        size_t totalRows = 0;
        for (const auto& file : inputFiles) {
            size_t beforePos = outfile.tellp();
            writeRowsFromFile(file, outfile, headers);
            size_t afterPos = outfile.tellp();
            // Estimate rows written (rough approximation)
            if (afterPos > beforePos) {
                totalRows += (afterPos - beforePos) / (headers.size() * 10);  // rough estimate
            }
        }
        
        outfile.close();
        std::cout << "Wrote CSV to " << outputFile << std::endl;
    }
    
    static void printConvertUsage(const char* progName) {
        std::cerr << "Usage: " << progName << " convert [options] <input_file(s)_or_directory(ies)> <output.csv>" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Convert JSON or CSV sensor data files to CSV format." << std::endl;
        std::cerr << "For JSON: Each line in input files should contain JSON with sensor readings." << std::endl;
        std::cerr << "For CSV: Files with .csv extension are automatically detected and processed." << std::endl;
        std::cerr << "Each sensor reading will become a row in the output CSV." << std::endl;
        std::cerr << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << "  -r, --recursive           Recursively process subdirectories" << std::endl;
        std::cerr << "  -v                        Verbose output (show progress)" << std::endl;
        std::cerr << "  -V                        Very verbose output (show detailed progress)" << std::endl;
        std::cerr << "  -e, --extension <ext>     Filter files by extension (e.g., .out or out)" << std::endl;
        std::cerr << "  -d, --depth <n>           Maximum recursion depth (0 = current dir only)" << std::endl;
        std::cerr << "  --use-prototype           Use sc-prototype command to define columns" << std::endl;
        std::cerr << "  --not-empty <column>      Skip rows where column is empty (can be used multiple times)" << std::endl;
        std::cerr << "  --only-value <col:val>    Only include rows where column has specific value (can be used multiple times)" << std::endl;
        std::cerr << "  --remove-errors           Remove error readings (DS18B20 temperature=85)" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Examples:" << std::endl;
        std::cerr << "  " << progName << " convert sensor1.out output.csv" << std::endl;
        std::cerr << "  " << progName << " convert sensor1.csv sensor2.csv output.csv" << std::endl;
        std::cerr << "  " << progName << " convert sensor1.out sensor2.out output.csv" << std::endl;
        std::cerr << "  " << progName << " convert --remove-errors sensor1.out output.csv" << std::endl;
        std::cerr << "  " << progName << " convert -e .out /path/to/sensor/dir output.csv" << std::endl;
        std::cerr << "  " << progName << " convert -r -e .csv /path/to/sensor/dir output.csv" << std::endl;
        std::cerr << "  " << progName << " convert -r -e .out /path/to/sensor/dir output.csv" << std::endl;
        std::cerr << "  " << progName << " convert -r -d 2 -e .out /path/to/logs output.csv" << std::endl;
        std::cerr << "  " << progName << " convert --use-prototype -r -e .out /path/to/logs output.csv" << std::endl;
        std::cerr << "  " << progName << " convert --not-empty unit --not-empty value -e .out /logs output.csv" << std::endl;
        std::cerr << "  " << progName << " convert --only-value type:temperature -r -e .out /logs output.csv" << std::endl;
        std::cerr << "  " << progName << " convert --only-value type:temperature --only-value unit:C /logs output.csv" << std::endl;
        std::cerr << "  " << progName << " convert --only-value type:temperature -r -e .out /logs output.csv" << std::endl;
        std::cerr << "  " << progName << " convert --only-value type:temperature --only-value unit:C /logs output.csv" << std::endl;
    }
};

class ErrorLister {
private:
    std::vector<std::string> inputFiles;
    bool recursive;
    std::string extensionFilter;
    int maxDepth;
    int verbosity;
    
    // Helper to detect if a file is CSV based on extension
    bool isCsvFile(const std::string& filename) {
        size_t dotPos = filename.find_last_of('.');
        if (dotPos == std::string::npos) {
            return false;
        }
        std::string ext = filename.substr(dotPos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".csv";
    }
    
    bool isDirectory(const std::string& path) {
        struct stat info;
        if (stat(path.c_str(), &info) != 0) {
            return false;
        }
        return (info.st_mode & S_IFDIR) != 0;
    }
    
    bool matchesExtension(const std::string& filename) {
        if (extensionFilter.empty()) {
            return true;
        }
        size_t dotPos = filename.find_last_of('.');
        if (dotPos == std::string::npos) {
            return false;
        }
        std::string ext = filename.substr(dotPos);
        return ext == extensionFilter;
    }
    
    // Helper to detect if a reading contains an error
    bool isErrorReading(const std::map<std::string, std::string>& reading) {
        // Check for DS18B20 sensor error (temperature = 85)
        auto typeIt = reading.find("type");
        auto valueIt = reading.find("value");
        auto tempIt = reading.find("temperature");
        
        // Check if type is ds18b20 or DS18B20 (case insensitive check)
        bool isDS18B20 = false;
        if (typeIt != reading.end()) {
            std::string type = typeIt->second;
            std::transform(type.begin(), type.end(), type.begin(), ::tolower);
            isDS18B20 = (type == "ds18b20");
        }
        
        if (isDS18B20) {
            // Check value field
            if (valueIt != reading.end() && valueIt->second == "85") {
                return true;
            }
            // Also check temperature field as fallback
            if (tempIt != reading.end() && tempIt->second == "85") {
                return true;
            }
        }
        
        return false;
    }
    
    void collectFilesFromDirectory(const std::string& dirPath, int currentDepth = 0) {
        // Check depth limit
        if (maxDepth >= 0 && currentDepth > maxDepth) {
            return;
        }
        
#ifdef _WIN32
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA((dirPath + "\\\\*").c_str(), &findData);
        
        if (hFind == INVALID_HANDLE_VALUE) {
            std::cerr << "Warning: Cannot open directory: " << dirPath << std::endl;
            return;
        }
        
        do {
            std::string filename = findData.cFileName;
            if (filename != "." && filename != "..") {
                std::string fullPath = dirPath + "\\\\" + filename;
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (recursive) {
                        collectFilesFromDirectory(fullPath, currentDepth + 1);
                    }
                } else {
                    if (matchesExtension(filename)) {
                        inputFiles.push_back(fullPath);
                    }
                }
            }
        } while (FindNextFileA(hFind, &findData) != 0);
        
        FindClose(hFind);
#else
        DIR* dir = opendir(dirPath.c_str());
        if (!dir) {
            std::cerr << "Warning: Cannot open directory: " << dirPath << std::endl;
            return;
        }
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string filename = entry->d_name;
            if (filename != "." && filename != "..") {
                std::string fullPath = dirPath + "/" + filename;
                if (isDirectory(fullPath)) {
                    if (recursive) {
                        collectFilesFromDirectory(fullPath, currentDepth + 1);
                    }
                } else {
                    if (matchesExtension(filename)) {
                        inputFiles.push_back(fullPath);
                    }
                }
            }
        }
        closedir(dir);
#endif
    }
    
    void listErrorsFromFile(const std::string& filename) {
        std::ifstream infile(filename);
        if (!infile) {
            std::cerr << "Warning: Cannot open file: " << filename << std::endl;
            return;
        }
        
        std::string line;
        int lineNum = 0;
        
        // Check if this is a CSV file
        if (isCsvFile(filename)) {
            // CSV format
            std::vector<std::string> csvHeaders;
            if (std::getline(infile, line) && !line.empty()) {
                lineNum++;
                bool needMore = false;
                csvHeaders = CsvParser::parseCsvLine(infile, line, needMore);
            }
            
            while (std::getline(infile, line)) {
                lineNum++;
                if (line.empty()) continue;
                
                bool needMore = false;
                auto fields = CsvParser::parseCsvLine(infile, line, needMore);
                if (fields.empty()) continue;
                
                // Create a map from CSV headers to values
                std::map<std::string, std::string> reading;
                for (size_t i = 0; i < std::min(csvHeaders.size(), fields.size()); ++i) {
                    reading[csvHeaders[i]] = fields[i];
                }
                
                if (isErrorReading(reading)) {
                    std::cout << filename << ":" << lineNum;
                    // Print relevant fields
                    auto typeIt = reading.find("type");
                    auto valueIt = reading.find("value");
                    auto tempIt = reading.find("temperature");
                    auto idIt = reading.find("sensor_id");
                    auto nameIt = reading.find("name");
                    
                    if (typeIt != reading.end()) std::cout << " type=" << typeIt->second;
                    if (idIt != reading.end()) std::cout << " sensor_id=" << idIt->second;
                    if (nameIt != reading.end()) std::cout << " name=" << nameIt->second;
                    if (valueIt != reading.end()) std::cout << " value=" << valueIt->second;
                    if (tempIt != reading.end()) std::cout << " temperature=" << tempIt->second;
                    std::cout << std::endl;
                }
            }
        } else {
            // JSON format
            while (std::getline(infile, line)) {
                lineNum++;
                if (line.empty()) continue;
                
                auto readings = JsonParser::parseJsonLine(line);
                for (const auto& reading : readings) {
                    if (reading.empty()) continue;
                    
                    if (isErrorReading(reading)) {
                        std::cout << filename << ":" << lineNum;
                        // Print relevant fields
                        auto typeIt = reading.find("type");
                        auto valueIt = reading.find("value");
                        auto tempIt = reading.find("temperature");
                        auto idIt = reading.find("sensor_id");
                        auto nameIt = reading.find("name");
                        
                        if (typeIt != reading.end()) std::cout << " type=" << typeIt->second;
                        if (idIt != reading.end()) std::cout << " sensor_id=" << idIt->second;
                        if (nameIt != reading.end()) std::cout << " name=" << nameIt->second;
                        if (valueIt != reading.end()) std::cout << " value=" << valueIt->second;
                        if (tempIt != reading.end()) std::cout << " temperature=" << tempIt->second;
                        std::cout << std::endl;
                    }
                }
            }
        }
        
        infile.close();
    }
    
public:
    ErrorLister(int argc, char* argv[]) : recursive(false), extensionFilter(""), maxDepth(-1), verbosity(0) {
        // Parse flags and arguments
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "-r" || arg == "--recursive") {
                recursive = true;
            } else if (arg == "-v") {
                verbosity = 1;
            } else if (arg == "-V") {
                verbosity = 2;
            } else if (arg == "-e" || arg == "--extension") {
                if (i + 1 < argc) {
                    ++i;
                    extensionFilter = argv[i];
                    if (!extensionFilter.empty() && extensionFilter[0] != '.') {
                        extensionFilter = "." + extensionFilter;
                    }
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    exit(1);
                }
            } else if (arg == "-d" || arg == "--depth") {
                if (i + 1 < argc) {
                    ++i;
                    try {
                        maxDepth = std::stoi(argv[i]);
                        if (maxDepth < 0) {
                            std::cerr << "Error: depth must be non-negative" << std::endl;
                            exit(1);
                        }
                    } catch (...) {
                        std::cerr << "Error: invalid depth value '" << argv[i] << "'" << std::endl;
                        exit(1);
                    }
                } else {
                    std::cerr << "Error: " << arg << " requires an argument" << std::endl;
                    exit(1);
                }
            } else if (arg[0] == '-') {
                std::cerr << "Error: Unknown option '" << arg << "'" << std::endl;
                printListErrorsUsage(argv[0]);
                exit(1);
            } else {
                // It's a file or directory path
                if (isDirectory(arg)) {
                    collectFilesFromDirectory(arg);
                } else {
                    inputFiles.push_back(arg);
                }
            }
        }
        
        if (inputFiles.empty()) {
            std::cerr << "Error: No input files found" << std::endl;
            exit(1);
        }
    }
    
    void listErrors() {
        for (const auto& file : inputFiles) {
            listErrorsFromFile(file);
        }
    }
    
    static void printListErrorsUsage(const char* progName) {
        std::cerr << "Usage: " << progName << " list-errors [options] <input_file(s)_or_directory(ies)>" << std::endl;
        std::cerr << std::endl;
        std::cerr << "List error readings in sensor data files." << std::endl;
        std::cerr << "Currently detects DS18B20 sensors with temperature/value of 85 (error condition)." << std::endl;
        std::cerr << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << "  -r, --recursive           Recursively process subdirectories" << std::endl;
        std::cerr << "  -v                        Verbose output" << std::endl;
        std::cerr << "  -V                        Very verbose output" << std::endl;
        std::cerr << "  -e, --extension <ext>     Filter files by extension (e.g., .out or out)" << std::endl;
        std::cerr << "  -d, --depth <n>           Maximum recursion depth (0 = current dir only)" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Examples:" << std::endl;
        std::cerr << "  " << progName << " list-errors sensor1.out" << std::endl;
        std::cerr << "  " << progName << " list-errors -r -e .out /path/to/logs/" << std::endl;
        std::cerr << "  " << progName << " list-errors sensor1.csv sensor2.out" << std::endl;
    }
};

void printUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " <command> [options]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Commands:" << std::endl;
    std::cerr << "  convert       Convert JSON or CSV sensor data files to CSV" << std::endl;
    std::cerr << "  list-errors   List error readings in sensor data files" << std::endl;
    std::cerr << std::endl;
    std::cerr << "For command-specific help, use:" << std::endl;
    std::cerr << "  " << progName << " <command> --help" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string command = argv[1];
    
    if (command == "convert") {
        try {
            // Create new argv for converter (skip "convert" command)
            int newArgc = argc - 1;
            char** newArgv = new char*[newArgc];
            newArgv[0] = argv[0];  // program name
            for (int i = 2; i < argc; ++i) {
                newArgv[i - 1] = argv[i];
            }
            
            SensorDataConverter converter(newArgc, newArgv);
            converter.convert();
            
            delete[] newArgv;
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    } else if (command == "list-errors") {
        try {
            // Create new argv for error lister (skip "list-errors" command)
            int newArgc = argc - 1;
            char** newArgv = new char*[newArgc];
            newArgv[0] = argv[0];  // program name
            for (int i = 2; i < argc; ++i) {
                newArgv[i - 1] = argv[i];
            }
            
            ErrorLister lister(newArgc, newArgv);
            lister.listErrors();
            
            delete[] newArgv;
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    } else if (command == "--help" || command == "-h") {
        printUsage(argv[0]);
        return 0;
    } else {
        std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
        std::cerr << std::endl;
        return 1;
    }
}
