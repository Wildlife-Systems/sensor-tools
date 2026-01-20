#include "file_utils.h"
#include "compat/stat.h"
#include <algorithm>
#include <fstream>
#include <deque>
#include <iostream>
#include <cerrno>
#include <cstring>

bool FileUtils::isDirectory(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        std::cerr << "DEBUG isDirectory: stat failed for '" << path << "': " << strerror(errno) << std::endl;
        return false;
    }
    bool isDir = (info.st_mode & S_IFDIR) != 0;
    std::cerr << "DEBUG isDirectory: '" << path << "' -> " << (isDir ? "true" : "false") << std::endl;
    return isDir;
}

long long FileUtils::getFileSize(const std::string& filename) {
    struct stat info;
    if (stat(filename.c_str(), &info) != 0) {
        return -1;
    }
    return static_cast<long long>(info.st_size);
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

std::vector<std::string> FileUtils::readTailLines(const std::string& filename, int n) {
    std::vector<std::string> result;
    if (n <= 0) {
        return result;
    }
    
    std::ifstream file(filename);
    if (!file) {
        return result;
    }
    
    // Use a deque to efficiently keep only the last n lines
    std::deque<std::string> buffer;
    std::string line;
    
    while (std::getline(file, line)) {
        buffer.push_back(line);
        if (static_cast<int>(buffer.size()) > n) {
            buffer.pop_front();
        }
    }
    
    // Convert deque to vector
    result.reserve(buffer.size());
    for (const auto& l : buffer) {
        result.push_back(l);
    }
    
    return result;
}
