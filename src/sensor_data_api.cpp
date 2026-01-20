#include "sensor_data_api.h"
#include "data_reader.h"
#include "file_collector.h"
#include "date_utils.h"

#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>

extern "C" {

sensor_data_result_t *sensor_data_tail_by_sensor_id(
    const char *directory,
    const char *sensor_id,
    int max_count,
    int recursive
)
{
    if (!directory || !sensor_id || max_count <= 0) {
        return nullptr;
    }
    
    // Collect .out files from directory
    FileCollector collector(recursive != 0, ".out", -1, 0);
    collector.addPath(directory);
    std::vector<std::string> files = collector.getSortedFiles();
    
    if (files.empty()) {
        return nullptr;
    }
    
    // Create DataReader with tail-column-value filter settings
    // We need to read all files and filter by sensor_id, then take last max_count
    DataReader reader(0, "auto", 0);
    
    // Set up filter: only include rows where sensor_id matches
    reader.getFilter().addOnlyValueFilter("sensor_id", sensor_id);
    
    // Collect all matching readings from all files
    struct ValueTimestamp {
        double value;
        long timestamp;
    };
    std::vector<ValueTimestamp> allValues;
    allValues.reserve(max_count * 2);
    
    for (const auto& file : files) {
        reader.processFile(file, [&](const Reading& reading, int, const std::string&) {
            // Extract value
            auto valueIt = reading.find("value");
            if (valueIt == reading.end()) return;
            
            double val;
            try {
                val = std::stod(valueIt->second);
            } catch (...) {
                return; // Skip non-numeric values
            }
            
            // Extract timestamp if available
            long ts = static_cast<long>(DateUtils::getTimestamp(reading));
            
            allValues.push_back({val, ts});
        });
    }
    
    if (allValues.empty()) {
        return nullptr;
    }
    
    // Sort by timestamp if available, to ensure chronological order
    bool hasTimestamps = false;
    for (const auto& vt : allValues) {
        if (vt.timestamp > 0) {
            hasTimestamps = true;
            break;
        }
    }
    
    if (hasTimestamps) {
        std::sort(allValues.begin(), allValues.end(), 
                  [](const ValueTimestamp& a, const ValueTimestamp& b) {
                      return a.timestamp < b.timestamp;
                  });
    }
    
    // Take the last max_count values
    size_t startIdx = 0;
    if (allValues.size() > static_cast<size_t>(max_count)) {
        startIdx = allValues.size() - max_count;
    }
    int resultCount = static_cast<int>(allValues.size() - startIdx);
    
    // Allocate result structure
    sensor_data_result_t *result = static_cast<sensor_data_result_t*>(
        malloc(sizeof(sensor_data_result_t)));
    if (!result) return nullptr;
    
    result->values = static_cast<double*>(malloc(resultCount * sizeof(double)));
    result->timestamps = static_cast<long*>(malloc(resultCount * sizeof(long)));
    result->count = resultCount;
    
    if (!result->values || !result->timestamps) {
        free(result->values);
        free(result->timestamps);
        free(result);
        return nullptr;
    }
    
    // Copy values (oldest first)
    for (int i = 0; i < resultCount; i++) {
        result->values[i] = allValues[startIdx + i].value;
        result->timestamps[i] = allValues[startIdx + i].timestamp;
    }
    
    return result;
}

sensor_data_result_t *sensor_data_range_by_sensor_id(
    const char *directory,
    const char *sensor_id,
    long start_time,
    long end_time,
    int recursive
)
{
    if (!directory || !sensor_id || start_time >= end_time) {
        return nullptr;
    }
    
    // Collect .out files from directory
    FileCollector collector(recursive != 0, ".out", -1, 0);
    collector.addPath(directory);
    std::vector<std::string> files = collector.getSortedFiles();
    
    if (files.empty()) {
        return nullptr;
    }
    
    // Create DataReader with filter for sensor_id
    DataReader reader(0, "auto", 0);
    reader.getFilter().addOnlyValueFilter("sensor_id", sensor_id);
    
    // Collect all matching readings within time range
    struct ValueTimestamp {
        double value;
        long timestamp;
    };
    std::vector<ValueTimestamp> allValues;
    
    for (const auto& file : files) {
        reader.processFile(file, [&](const Reading& reading, int, const std::string&) {
            // Extract timestamp
            long ts = static_cast<long>(DateUtils::getTimestamp(reading));
            
            // Filter by time range
            if (ts < start_time || ts > end_time) return;
            
            // Extract value
            auto valueIt = reading.find("value");
            if (valueIt == reading.end()) return;
            
            double val;
            try {
                val = std::stod(valueIt->second);
            } catch (...) {
                return; // Skip non-numeric values
            }
            
            allValues.push_back({val, ts});
        });
    }
    
    if (allValues.empty()) {
        return nullptr;
    }
    
    // Sort by timestamp
    std::sort(allValues.begin(), allValues.end(), 
              [](const ValueTimestamp& a, const ValueTimestamp& b) {
                  return a.timestamp < b.timestamp;
              });
    
    int resultCount = static_cast<int>(allValues.size());
    
    // Allocate result structure
    sensor_data_result_t *result = static_cast<sensor_data_result_t*>(
        malloc(sizeof(sensor_data_result_t)));
    if (!result) return nullptr;
    
    result->values = static_cast<double*>(malloc(resultCount * sizeof(double)));
    result->timestamps = static_cast<long*>(malloc(resultCount * sizeof(long)));
    result->count = resultCount;
    
    if (!result->values || !result->timestamps) {
        free(result->values);
        free(result->timestamps);
        free(result);
        return nullptr;
    }
    
    // Copy values (oldest first)
    for (int i = 0; i < resultCount; i++) {
        result->values[i] = allValues[i].value;
        result->timestamps[i] = allValues[i].timestamp;
    }
    
    return result;
}

void sensor_data_result_free(sensor_data_result_t *result)
{
    if (!result) return;
    free(result->values);
    free(result->timestamps);
    free(result);
}

} // extern "C"
