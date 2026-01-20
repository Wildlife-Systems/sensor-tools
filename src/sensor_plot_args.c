/* sensor_plot_args.c - Command-line argument parsing for sensor-plot */

#include "sensor_plot_args.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Helper to duplicate a string */
static char *safe_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *dup = malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

/* Helper to set error message */
static void set_error(sensor_plot_args_t *args, const char *msg) {
    free(args->error_message);
    args->error_message = safe_strdup(msg);
}

int sensor_plot_args_parse(int argc, char **argv, sensor_plot_args_t *args) {
    if (!args) return -1;
    
    sensor_plot_args_init(args);
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--sensor") == 0) {
            if (i + 1 >= argc) {
                set_error(args, "--sensor requires an argument");
                return -1;
            }
            if (args->num_sensors >= SENSOR_PLOT_MAX_SENSORS) {
                char msg[64];
                snprintf(msg, sizeof(msg), "maximum %d sensors allowed", SENSOR_PLOT_MAX_SENSORS);
                set_error(args, msg);
                return -1;
            }
            i++;
            args->sensor_ids[args->num_sensors] = safe_strdup(argv[i]);
            args->num_sensors++;
            
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--depth") == 0) {
            if (i + 1 >= argc) {
                set_error(args, "-d/--depth requires an argument");
                return -1;
            }
            i++;
            args->max_depth = atoi(argv[i]);
            if (args->max_depth < 0) {
                set_error(args, "-d/--depth requires a non-negative number");
                return -1;
            }
            
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--recursive") == 0) {
            args->recursive = 1;
            
        } else if (strcmp(argv[i], "-R") == 0 || strcmp(argv[i], "--no-recursive") == 0) {
            args->recursive = 0;
            
        } else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--extension") == 0) {
            if (i + 1 >= argc) {
                set_error(args, "-e/--extension requires an argument");
                return -1;
            }
            i++;
            free(args->extension);
            /* Ensure extension starts with a dot */
            if (argv[i][0] == '.') {
                args->extension = safe_strdup(argv[i]);
            } else {
                args->extension = malloc(strlen(argv[i]) + 2);
                if (args->extension) {
                    args->extension[0] = '.';
                    strcpy(args->extension + 1, argv[i]);
                }
            }
            
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            args->show_help = 1;
            return 1;
            
        } else if (argv[i][0] != '-') {
            /* Positional argument - treat as data directory/file */
            free(args->data_directory);
            args->data_directory = safe_strdup(argv[i]);
            
        } else {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown option: %s", argv[i]);
            set_error(args, msg);
            return -1;
        }
    }
    
    if (args->num_sensors == 0) {
        set_error(args, "at least one --sensor argument required");
        return -1;
    }
    
    return 0;
}
