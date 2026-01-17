#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <string>
#include <vector>

class FileUtils {
public:
    static bool isDirectory(const std::string& path);
    static bool isCsvFile(const std::string& filename);
    static bool matchesExtension(const std::string& filename, const std::string& extensionFilter);
};

#endif // FILE_UTILS_H
