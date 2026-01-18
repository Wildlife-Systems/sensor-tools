#ifndef DATA_COUNTER_H
#define DATA_COUNTER_H

#include <string>
#include <vector>

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
 */
class DataCounter : public CommandBase {
private:
    bool followMode;  // --follow flag for continuous monitoring
    
    /**
     * Count readings from a single file
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
