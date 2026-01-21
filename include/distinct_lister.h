#ifndef DISTINCT_LISTER_H
#define DISTINCT_LISTER_H

#include <string>
#include <vector>
#include <set>
#include <mutex>

#include "command_base.h"

/**
 * DistinctLister - List unique values in a specified column.
 * 
 * Supports:
 * - Listing distinct values from files or stdin
 * - All standard filters (date range, value-based, error removal)
 * - Recursive directory processing
 * - Multiple output formats (plain, csv, json)
 * - Multi-threaded file processing
 */
class DistinctLister : public CommandBase {
private:
    std::string columnName;           // Column to get distinct values from
    std::string outputFormat;         // --output-format: plain, csv, json
    bool showCounts;                  // --counts: show value counts
    std::set<std::string> distinctValues;  // Unique values found
    std::map<std::string, long long> valueCounts;  // Counts per value (when --counts)
    std::mutex valuesMutex;           // Mutex for thread-safe access
    
    /**
     * Collect distinct values from a single file
     */
    void collectFromFile(const std::string& filename);
    
    /**
     * Collect distinct values from stdin
     */
    void collectFromStdin();
    
    /**
     * Output the collected distinct values
     */
    void outputResults();

public:
    /**
     * Construct from command line arguments
     */
    DistinctLister(int argc, char* argv[]);
    
    /**
     * Execute the distinct listing
     */
    void listDistinct();
    
    /**
     * Print usage information
     */
    static void printDistinctUsage(const char* progName);
};

#endif // DISTINCT_LISTER_H
