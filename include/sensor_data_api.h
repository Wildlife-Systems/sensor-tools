#ifndef SENSOR_DATA_API_H
#define SENSOR_DATA_API_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * C-compatible API for sensor-data functionality.
 * Allows C programs (like sensor-mon) to use sensor-data features
 * without spawning a subprocess.
 */

/**
 * Result structure for reading sensor values
 */
typedef struct {
    double *values;      /* Array of values */
    long *timestamps;    /* Array of unix timestamps (or 0 if not available) */
    int count;           /* Number of values returned */
} sensor_data_result_t;

/**
 * Read the last n values for a specific sensor_id from .out files in a directory.
 * 
 * @param directory     Directory to search (e.g., "/var/ws")
 * @param sensor_id     The sensor_id to filter by
 * @param max_count     Maximum number of values to return
 * @param recursive     If non-zero, search subdirectories
 * @return              Result structure (caller must free with sensor_data_result_free)
 *                      Returns NULL on error
 */
sensor_data_result_t *sensor_data_tail_by_sensor_id(
    const char *directory,
    const char *sensor_id,
    int max_count,
    int recursive
);

/**
 * Free a result structure returned by sensor_data_* functions
 */
void sensor_data_result_free(sensor_data_result_t *result);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_DATA_API_H */
