#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <string>
#include <vector>

class FileUtils {
public:
    static bool isDirectory(const std::string& path);
    static bool isCsvFile(const std::string& filename);
    static bool matchesExtension(const std::string& filename, const std::string& extensionFilter);
    
    /**
     * Read the last n lines from a file.
     * Returns the lines in order (first to last).
     * If the file has fewer than n lines, returns all lines.
     * 
     * @param filename Path to the file
     * @param n Number of lines to read from the end
     * @return Vector of strings, each being a line (without newline)
     */
    static std::vector<std::string> readTailLines(const std::string& filename, int n);
};

#endif // FILE_UTILS_H
