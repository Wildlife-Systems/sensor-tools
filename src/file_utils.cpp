#include "file_utils.h"
#include <sys/stat.h>
#include <algorithm>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

bool FileUtils::isDirectory(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}

bool FileUtils::isCsvFile(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return false;
    }
    std::string ext = filename.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".csv";
}

bool FileUtils::matchesExtension(const std::string& filename, const std::string& extensionFilter) {
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

void FileUtils::collectFilesFromDirectory(const std::string& dirPath, 
                                         std::vector<std::string>& files,
                                         bool recursive,
                                         const std::string& extensionFilter,
                                         int maxDepth,
                                         int currentDepth,
                                         int verbosity) {
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
                    collectFilesFromDirectory(fullPath, files, recursive, extensionFilter, maxDepth, currentDepth + 1, verbosity);
                }
            } else {
                if (matchesExtension(filename, extensionFilter)) {
                    if (verbosity >= 2) {
                        std::cout << "  Found file: " << fullPath << std::endl;
                    }
                    files.push_back(fullPath);
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
                    collectFilesFromDirectory(fullPath, files, recursive, extensionFilter, maxDepth, currentDepth + 1, verbosity);
                }
            } else {
                if (matchesExtension(filename, extensionFilter)) {
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
#endif
}
