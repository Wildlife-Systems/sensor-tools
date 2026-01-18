#ifndef ERROR_LISTER_H
#define ERROR_LISTER_H

#include <string>
#include <vector>
#include <map>

#include "command_base.h"

/**
 * ErrorLister - List error readings in sensor data files.
 * 
 * Supports:
 * - Detecting DS18B20 sensor errors (value=85 or -127)
 * - Date range filtering
 * - Recursive directory processing
 */
class ErrorLister : public CommandBase {
public:
    /**
     * Construct lister from command line arguments
     */
    ErrorLister(int argc, char* argv[]);
    
    /**
     * Helper to print a single error line
     */
    static void printErrorLine(const std::map<std::string, std::string>& reading, 
                               int lineNum, const std::string& source);
    
    /**
     * Execute the error listing
     */
    void listErrors();
    
    /**
     * Print usage information
     */
    static void printListErrorsUsage(const char* progName);
};

#endif // ERROR_LISTER_H
