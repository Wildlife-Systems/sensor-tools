#ifndef FILE_COLLECTOR_H
#define FILE_COLLECTOR_H

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "compat/dirent.h"
#include "file_utils.h"

// Centralized file collector that handles directory traversal and extension filtering
class FileCollector {
private:
    std::vector<std::string> files;
    bool recursive;
    std::string extensionFilter;
    int maxDepth;
    int verbosity;
    
    void collectFromDirectory(const std::string& dirPath, int currentDepth = 0) {
        if (verbosity >= 1) {
            std::cout << "Scanning directory: " << dirPath << " (depth " << currentDepth << ")" << std::endl;
        }
        
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
                if (FileUtils::isDirectory(fullPath)) {
                    // Check depth limit BEFORE recursing to avoid unnecessary work
                    if (recursive && (maxDepth < 0 || currentDepth < maxDepth)) {
                        collectFromDirectory(fullPath, currentDepth + 1);
                    } else if (verbosity >= 2 && recursive && maxDepth >= 0) {
                        std::cout << "  Skipping subdirectory (depth limit): " << fullPath << std::endl;
                    }
                } else {
                    if (FileUtils::matchesExtension(filename, extensionFilter)) {
                        if (verbosity >= 2) {
                            std::cout << "  Found file: " << fullPath << std::endl;
                        }
                        files.push_back(fullPath);
                    } else if (verbosity >= 2 && !extensionFilter.empty()) {
                        std::cout << "  Skipping (extension): " << fullPath << std::endl;
                    }
                }
            }
        }
        closedir(dir);
    }
    
public:
    FileCollector(bool recursive = false, const std::string& extension = "", int maxDepth = -1, int verbosity = 0)
        : recursive(recursive), extensionFilter(extension), maxDepth(maxDepth), verbosity(verbosity) {}
    
    void addPath(const std::string& path) {
        if (FileUtils::isDirectory(path)) {
            collectFromDirectory(path);
        } else {
            files.push_back(path);
        }
    }
    
    const std::vector<std::string>& getFiles() const {
        return files;
    }
    
    // Get sorted files for deterministic processing
    std::vector<std::string> getSortedFiles() {
        std::vector<std::string> sortedFiles = files;
        std::sort(sortedFiles.begin(), sortedFiles.end());
        return sortedFiles;
    }
};

#endif // FILE_COLLECTOR_H
