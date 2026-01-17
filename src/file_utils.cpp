#include "file_utils.h"
#include <sys/stat.h>
#include <algorithm>

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
