#ifndef FILE_COLLECTOR_H
#define FILE_COLLECTOR_H

#include <iostream>
#include <string>
#include <vector>
#include <dirent.h>
#include "file_utils.h"

// Centralized file collector that handles directory traversal and extension filtering
class FileCollector {
private:
    std::vector<std::string> files;
    bool recursive;
    std::string extensionFilter;
    int maxDepth;
    int verbosity;
    
    bool matchesExtension(const std::string& filename) const {
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
    
    void collectFromDirectory(const std::string& dirPath, int currentDepth = 0) {
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
                    if (recursive) {
                        collectFromDirectory(fullPath, currentDepth + 1);
                    }
                } else {
                    if (matchesExtension(filename)) {
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
    
    size_t size() const {
        return files.size();
    }
    
    bool empty() const {
        return files.empty();
    }
};

#endif // FILE_COLLECTOR_H
