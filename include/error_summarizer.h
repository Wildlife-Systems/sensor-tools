#ifndef ERROR_SUMMARIZER_H
#define ERROR_SUMMARIZER_H

#include <string>
#include <vector>
#include <map>

#include "command_base.h"

/**
 * ErrorSummarizer - Summarise error readings with counts.
 * 
 * Supports:
 * - Detecting DS18B20 sensor errors (value=85 or -127)
 * - Counting occurrences by error type
 * - Date range filtering
 * - Recursive directory processing
 */
class ErrorSummarizer : public CommandBase {
private:
    std::map<std::string, int> errorCounts;

public:
    /**
     * Construct summarizer from command line arguments
     */
    ErrorSummarizer(int argc, char* argv[]);
    
    /**
     * Execute the error summarization
     */
    void summariseErrors();
    
    /**
     * Print usage information
     */
    static void printSummariseErrorsUsage(const char* progName);
};

#endif // ERROR_SUMMARIZER_H
