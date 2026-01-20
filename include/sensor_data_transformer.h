#ifndef SENSOR_DATA_TRANSFORMER_H
#define SENSOR_DATA_TRANSFORMER_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <mutex>

#include "command_base.h"

/**
 * SensorDataTransformer - Transform sensor data between JSON and CSV formats.
 * 
 * Supports:
 * - JSON to CSV conversion
 * - JSON to JSON (streaming passthrough with optional filtering)
 * - CSV to JSON conversion
 * - Date range filtering
 * - Value-based filtering (include/exclude)
 * - Error reading removal
 * - Recursive directory processing
 * - Multi-threaded column discovery
 */
class SensorDataTransformer : public CommandBase {
private:
    // Output configuration
    std::string outputFile;
    std::string outputFormat;  // "json" or "csv"
    bool removeWhitespace;
    bool rejectMode;  // If true, output rejected readings instead of accepted
    
    // Column discovery
    std::set<std::string> allKeys;
    std::mutex keysMutex;
    std::mutex outputMutex;  // Protect console output in multi-threaded operations
    int numThreads;
    bool usePrototype;
    
    /**
     * Check if any filtering is active (affects whether we can pass-through JSON lines)
     */
    bool hasActiveFilters() const;
    
    /**
     * Execute sc-prototype command and parse columns from JSON output
     */
    bool getPrototypeColumns();
    
    /**
     * First pass: collect all column names from a file (thread-safe)
     */
    void collectKeysFromFile(const std::string& filename);
    
    /**
     * Second pass: write rows from a file directly to CSV
     */
    void writeRowsFromFile(const std::string& filename, std::ostream& outfile, 
                           const std::vector<std::string>& headers);
    
    /**
     * Write rows from a file as JSON - preserves line-by-line array format of .out files
     */
    void writeRowsFromFileJson(const std::string& filename, std::ostream& outfile, 
                               bool& firstOutput);
    
    /**
     * Process collected readings and output as CSV
     */
    void processStdinData(const ReadingList& readings, 
                          const std::vector<std::string>& headers, 
                          std::ostream& outfile);
    
    /**
     * Process collected readings and output as JSON array
     */
    void processStdinDataJson(const ReadingList& readings, 
                              std::ostream& outfile);
    
    /**
     * Write a single row (dispatches to CSV or JSON based on outputFormat)
     */
    void writeRow(const Reading& reading,
                  const std::vector<std::string>& headers,
                  std::ostream& outfile);

public:
    /**
     * Construct transformer from command line arguments
     * @param rejectMode If true, output rejected readings instead of accepted
     */
    SensorDataTransformer(int argc, char* argv[], bool rejectMode = false);
    
    /**
     * Execute the transformation
     */
    void transform();
    
    /**
     * Print usage information
     */
    static void printTransformUsage(const char* progName);
    
    /**
     * Print usage information for list-rejects command
     */
    static void printListRejectsUsage(const char* progName);
};

#endif // SENSOR_DATA_TRANSFORMER_H
