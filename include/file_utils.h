#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <string>
#include <vector>
#include <algorithm>
#include <fstream>

class FileUtils {
public:
    static bool isDirectory(const std::string& path);
    static bool isCsvFile(const std::string& filename);
    static bool matchesExtension(const std::string& filename, const std::string& extensionFilter);
    
    /**
     * Get the size of a file in bytes.
     * Returns -1 if the file doesn't exist or can't be accessed.
     */
    static long long getFileSize(const std::string& filename);
    
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
    
    /**
     * Read backwards through a file, returning lines in reverse order.
     * Reads from end of file, yielding lines one at a time via callback.
     * Stops when callback returns false.
     * 
     * @param filename Path to the file
     * @param callback Function called with each line (in reverse order). Return false to stop.
     * @return Number of lines read
     */
    template<typename Callback>
    static int readLinesReverse(const std::string& filename, Callback callback);
};

// Template implementation
template<typename Callback>
int FileUtils::readLinesReverse(const std::string& filename, Callback callback) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file) {
        return 0;
    }
    
    std::streampos fileSize = file.tellg();
    if (fileSize == std::streampos(0)) {
        return 0;
    }
    
    int linesRead = 0;
    std::string currentLine;
    
    // Read backwards through the file
    for (std::streamoff pos = static_cast<std::streamoff>(fileSize) - 1; pos >= 0; --pos) {
        file.seekg(pos);
        char c;
        file.get(c);
        
        if (c == '\n') {
            if (!currentLine.empty()) {
                // Reverse the line (we built it backwards)
                std::reverse(currentLine.begin(), currentLine.end());
                linesRead++;
                if (!callback(currentLine)) {
                    return linesRead;
                }
                currentLine.clear();
            }
        } else if (c != '\r') {
            currentLine += c;
        }
    }
    
    // Handle the first line (no newline before it)
    if (!currentLine.empty()) {
        std::reverse(currentLine.begin(), currentLine.end());
        linesRead++;
        callback(currentLine);
    }
    
    return linesRead;
}

#endif // FILE_UTILS_H
