/* sensor_plot_args.h - Command-line argument parsing for sensor-plot
 * 
 * Separated from sensor-plot.c for unit testing.
 */

#ifndef SENSOR_PLOT_ARGS_H
#define SENSOR_PLOT_ARGS_H

#include <stdlib.h>

#define SENSOR_PLOT_MAX_SENSORS 5
#define SENSOR_PLOT_DEFAULT_DIR "/var/ws"

/* Parsed arguments structure */
typedef struct {
    char *sensor_ids[SENSOR_PLOT_MAX_SENSORS];
    int num_sensors;
    char *data_directory;      /* NULL means use default */
    int recursive;             /* 1 = recursive (default), 0 = not */
    int max_depth;             /* -1 = unlimited (default), >= 0 = limit */
    char *extension;           /* NULL means use default ".out" */
    int show_help;             /* 1 if --help was requested */
    char *error_message;       /* Non-NULL if parse error occurred */
} sensor_plot_args_t;

/* Initialize args structure with defaults */
static inline void sensor_plot_args_init(sensor_plot_args_t *args) {
    if (!args) return;
    for (int i = 0; i < SENSOR_PLOT_MAX_SENSORS; i++) {
        args->sensor_ids[i] = NULL;
    }
    args->num_sensors = 0;
    args->data_directory = NULL;
    args->recursive = 1;  /* Default: recursive */
    args->max_depth = -1; /* Default: unlimited */
    args->extension = NULL;
    args->show_help = 0;
    args->error_message = NULL;
}

/* Free memory allocated by args */
static inline void sensor_plot_args_free(sensor_plot_args_t *args) {
    if (!args) return;
    for (int i = 0; i < args->num_sensors; i++) {
        free(args->sensor_ids[i]);
        args->sensor_ids[i] = NULL;
    }
    free(args->data_directory);
    free(args->extension);
    free(args->error_message);
    args->data_directory = NULL;
    args->extension = NULL;
    args->error_message = NULL;
    args->num_sensors = 0;
}

/* Parse command line arguments
 * Returns 0 on success, -1 on error (check args->error_message),
 * 1 if help was shown (check args->show_help)
 */
int sensor_plot_args_parse(int argc, char **argv, sensor_plot_args_t *args);

#endif /* SENSOR_PLOT_ARGS_H */
