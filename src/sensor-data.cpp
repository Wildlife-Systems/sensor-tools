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

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

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
                value = line.substr(valueStart + 1, valueEnd - valueStart - 1);
                pos = valueEnd + 1;
            } else if (line[valueStart] == '[') {
                // Array - this might contain sensor readings
                size_t bracketEnd = line.find(']', valueStart);
                if (bracketEnd == std::string::npos) break;
                
                // If we have accumulated data and hit an array, might be multiple readings
                if (!current.empty() && key == "readings") {
                    // Parse array of readings
                    std::string arrayContent = line.substr(valueStart + 1, bracketEnd - valueStart - 1);
                    // For now, store the whole array
                    value = arrayContent;
                }
                pos = bracketEnd + 1;
            } else {
                // Numeric or other value
                size_t valueEnd = line.find_first_of(",}]", valueStart);
                if (valueEnd == std::string::npos) valueEnd = line.length();
                value = line.substr(valueStart, valueEnd - valueStart);
                // Trim whitespace
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
    std::vector<std::map<std::string, std::string>> allReadings;
    bool hasInputFiles;
    
    bool isDirectory(const std::string& path) {
        struct stat info;
        if (stat(path.c_str(), &info) != 0) {
            return false;
        }
        return (info.st_mode & S_IFDIR) != 0;
    }
    
    void collectFilesFromDirectory(const std::string& dirPath) {
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
                    collectFilesFromDirectory(fullPath);
                } else {
                    inputFiles.push_back(fullPath);
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
                    collectFilesFromDirectory(fullPath);
                } else {
                    inputFiles.push_back(fullPath);
                }
            }
        }
        closedir(dir);
#endif
    }
    
    void processFile(const std::string& filename) {
        std::ifstream infile(filename);
        if (!infile) {
            std::cerr << "Warning: Cannot open file: " << filename << std::endl;
            return;
        }
        
        std::string line;
        int lineNum = 0;
        while (std::getline(infile, line)) {
            lineNum++;
            if (line.empty()) continue;
            
            auto readings = JsonParser::parseJsonLine(line);
            for (const auto& reading : readings) {
                if (!reading.empty()) {
                    // Collect all keys
                    for (const auto& pair : reading) {
                        allKeys.insert(pair.first);
                    }
                    allReadings.push_back(reading);
                }
            }
        }
        
        infile.close();
    }
    
    void writeCSV() {
        std::ofstream outfile(outputFile);
        if (!outfile) {
            std::cerr << "Error: Cannot create output file: " << outputFile << std::endl;
            return;
        }
        
        // Convert set to vector for consistent ordering
        std::vector<std::string> headers(allKeys.begin(), allKeys.end());
        std::sort(headers.begin(), headers.end());
        
        // Write header
        for (size_t i = 0; i < headers.size(); ++i) {
            if (i > 0) outfile << ",";
            outfile << headers[i];
        }
        outfile << "\n";
        
        // Write data
        for (const auto& reading : allReadings) {
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
        
        outfile.close();
        std::cout << "Wrote " << allReadings.size() << " sensor readings to " << outputFile << std::endl;
    }
    
public:
    SensorDataConverter(int argc, char* argv[]) : hasInputFiles(false) {
        // argc should be at least 3 for "convert": program convert input output
        if (argc < 3) {
            printConvertUsage(argv[0]);
            exit(1);
        }
        
        // Last argument is output file
        outputFile = argv[argc - 1];
        
        // Arguments from index 1 to argc-2 are input files or directories
        // (index 0 is program name, we skip the "convert" command which was already processed)
        for (int i = 1; i < argc - 1; ++i) {
            std::string path = argv[i];
            if (isDirectory(path)) {
                collectFilesFromDirectory(path);
            } else {
                inputFiles.push_back(path);
            }
        }
        
        if (inputFiles.empty()) {
            std::cerr << "Error: No input files found" << std::endl;
            exit(1);
        }
        
        hasInputFiles = true;
    }
    
    void convert() {
        std::cout << "Processing " << inputFiles.size() << " file(s)..." << std::endl;
        
        for (const auto& file : inputFiles) {
            std::cout << "  Reading: " << file << std::endl;
            processFile(file);
        }
        
        std::cout << "Found " << allReadings.size() << " sensor readings with " 
                  << allKeys.size() << " unique fields" << std::endl;
        
        writeCSV();
    }
    
    static void printConvertUsage(const char* progName) {
        std::cerr << "Usage: " << progName << " convert <input_file(s)_or_directory(ies)> <output.csv>" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Convert JSON sensor data files to CSV format." << std::endl;
        std::cerr << "Each line in input files should contain JSON with sensor readings." << std::endl;
        std::cerr << "Each sensor reading will become a row in the output CSV." << std::endl;
        std::cerr << std::endl;
        std::cerr << "Examples:" << std::endl;
        std::cerr << "  " << progName << " convert sensor1.out output.csv" << std::endl;
        std::cerr << "  " << progName << " convert sensor1.out sensor2.out output.csv" << std::endl;
        std::cerr << "  " << progName << " convert /path/to/sensor/dir output.csv" << std::endl;
    }
};

void printUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " <command> [options]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Commands:" << std::endl;
    std::cerr << "  convert    Convert JSON sensor data files to CSV" << std::endl;
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
    } else if (command == "--help" || command == "-h") {
        printUsage(argv[0]);
        return 0;
    } else {
        std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
        std::cerr << std::endl;
        return 1;
    }
}
