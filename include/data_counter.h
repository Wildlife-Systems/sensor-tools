#ifndef DATA_COUNTER_H
#define DATA_COUNTER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

#include "command_base.h"

/**
 * DataCounter - Count sensor data readings with optional filters.
 * 
 * Supports:
 * - Counting readings from files or stdin
 * - Date range filtering
 * - Value-based filtering (include/exclude)
 * - Error reading removal
 * - Recursive directory processing
 * - Follow mode for files and stdin (like tail -f)
 * - Count by column value (--by-column)
 * - Multi-threaded file processing
 */
class DataCounter : public CommandBase {
private:
    bool followMode;  // --follow flag for continuous monitoring
    std::string byColumn;  // --by-column for counts per value
    std::string outputFormat;  // --output-format: human, csv, json
    std::unordered_map<std::string, long long> valueCounts;  // counts per column value
    std::mutex valueCountsMutex;  // mutex for thread-safe access to valueCounts
    
    /**
     * Count readings from a single file (returns count, updates valueCounts thread-safely)
     */
    long long countFromFile(const std::string& filename);
    
    /**
     * Count readings from stdin
     */
    long long countFromStdin();
    
    /**
     * Count readings from stdin with follow mode (like tail -f)
     */
    void countFromStdinFollow();
    
    /**
     * Count readings from a file with follow mode (like tail -f)
     */
    void countFromFileFollow(const std::string& filename);

public:
    /**
     * Construct counter from command line arguments
     */
    DataCounter(int argc, char* argv[]);
    
    /**
     * Execute the counting
     */
    void count();
    
    /**
     * Print usage information
     */
    static void printCountUsage(const char* progName);
};

#endif // DATA_COUNTER_H
