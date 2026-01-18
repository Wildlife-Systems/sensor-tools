#ifndef STATS_ANALYSER_H
#define STATS_ANALYSER_H

#include <string>
#include <vector>
#include <map>

#include "command_base.h"

/**
 * StatsAnalyser - Calculate statistics for numeric sensor data.
 * 
 * Supports:
 * - Min, max, mean, median, standard deviation
 * - Filtering by column name
 * - Date range filtering
 * - Recursive directory processing
 */
class StatsAnalyser : public CommandBase {
private:
    std::string columnFilter;  // Specific column to analyze (empty = all, "value" = default)
    std::map<std::string, std::vector<double>> columnData;  // column name -> values
    
    /**
     * Check if a string is a valid numeric value
     */
    static bool isNumeric(const std::string& str);
    
    /**
     * Collect numeric data from a reading
     */
    void collectDataFromReading(const std::map<std::string, std::string>& reading);
    
    /**
     * Calculate median of a vector of values
     */
    static double calculateMedian(const std::vector<double>& values);
    
    /**
     * Calculate standard deviation of a vector of values
     */
    static double calculateStdDev(const std::vector<double>& values, double mean);

public:
    /**
     * Construct analyser from command line arguments
     */
    StatsAnalyser(int argc, char* argv[]);
    
    /**
     * Execute the analysis
     */
    void analyze();
    
    /**
     * Print usage information
     */
    static void printStatsUsage(const char* progName);
};

#endif // STATS_ANALYSER_H
