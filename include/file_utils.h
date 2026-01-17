#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <string>
#include <vector>

class FileUtils {
public:
    static bool isDirectory(const std::string& path);
    static bool isCsvFile(const std::string& filename);
    static void collectFilesFromDirectory(const std::string& dirPath, 
                                         std::vector<std::string>& files,
                                         bool recursive,
                                         const std::string& extensionFilter,
                                         int maxDepth,
                                         int currentDepth = 0,
                                         int verbosity = 0);
    static bool matchesExtension(const std::string& filename, const std::string& extensionFilter);
};

#endif // FILE_UTILS_H
