/* sensor-plot: Display historical sensor data graphs
 * 
 * Usage: sensor-plot --sensor sensor_id [--sensor sensor_id2] ...
 * 
 * Displays up to 5 sensor graphs on a single screen.
 * Day mode (default): shows last 24 hours, left/right arrows move by 1 hour
 * Week mode ('w'): arrows move by 1 day
 * Press 'q' to quit
 */

#ifdef _WIN32
    #include <curses.h>
    #include <windows.h>
    #define PATH_SEP ';'
#else
    #include <ncurses.h>
    #include <unistd.h>
    #define PATH_SEP ':'
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "graph.h"
#include "sensor_data_api.h"
#include "sensor_plot_args.h"

#define MAX_SENSORS SENSOR_PLOT_MAX_SENSORS
#define DEFAULT_DATA_DIR SENSOR_PLOT_DEFAULT_DIR

/* Mode definitions */
typedef enum {
    MODE_HOUR,
    MODE_DAY,
    MODE_WEEK,
    MODE_MONTH,
    MODE_YEAR
} view_mode_t;

/* Sensor data structure */
typedef struct {
    char *sensor_id;
    graph_data_t graph;
    int has_data;
} sensor_plot_t;

/* Global state */
static sensor_plot_t sensors[MAX_SENSORS];
static int num_sensors = 0;
static view_mode_t current_mode = MODE_DAY;
static time_t window_end;      /* End of current view window */
static volatile int running = 1;
static char *data_directory = NULL;  /* Data directory path */
static int recursive_search = 1;     /* Whether to search subdirectories */
static int max_depth = -1;           /* Max directory depth (-1 = unlimited) */
static char *extension_filter = NULL; /* File extension filter (e.g., ".out") */

/* Color pairs */
#define COLOR_FRAME 1
#define COLOR_DATA1 2
#define COLOR_DATA2 3
#define COLOR_DATA3 4
#define COLOR_DATA4 5
#define COLOR_DATA5 6
#define COLOR_LABEL 7

static int sensor_colors[] = {COLOR_DATA1, COLOR_DATA2, COLOR_DATA3, COLOR_DATA4, COLOR_DATA5};

/* Signal handler for clean exit */
static void handle_signal(int sig)
{
    (void)sig;
    running = 0;
}

/* Get window duration in seconds based on mode */
static long get_window_duration(void)
{
    switch (current_mode) {
        case MODE_HOUR:  return 3600;              /* 1 hour */
        case MODE_DAY:   return 24 * 3600;         /* 24 hours */
        case MODE_WEEK:  return 7 * 24 * 3600;     /* 7 days */
        case MODE_MONTH: return 30 * 24 * 3600;    /* 30 days */
        case MODE_YEAR:  return 365 * 24 * 3600;   /* 365 days */
        default:         return 24 * 3600;
    }
}

/* Get step size in seconds based on mode */
static long get_step_size(void)
{
    switch (current_mode) {
        case MODE_HOUR:  return 60;              /* 1 minute */
        case MODE_DAY:   return 3600;            /* 1 hour */
        case MODE_WEEK:  return 24 * 3600;       /* 1 day */
        case MODE_MONTH: return 7 * 24 * 3600;   /* 1 week */
        case MODE_YEAR:  return 30 * 24 * 3600;  /* 1 month */
        default:         return 3600;
    }
}

/* Get mode name for display */
static const char* get_mode_name(void)
{
    switch (current_mode) {
        case MODE_HOUR:  return "Hour (1h)";
        case MODE_DAY:   return "Day (24h)";
        case MODE_WEEK:  return "Week (7d)";
        case MODE_MONTH: return "Month (30d)";
        case MODE_YEAR:  return "Year (365d)";
        default:         return "Unknown";
    }
}

/* Load data for a sensor within the current time window */
static void load_sensor_data(int sensor_idx)
{
    if (sensor_idx < 0 || sensor_idx >= num_sensors) return;
    
    sensor_plot_t *s = &sensors[sensor_idx];
    reset_graph(&s->graph);
    s->has_data = 0;
    
    long start_time = (long)window_end - get_window_duration();
    long end_time = (long)window_end;
    
    const char *dir = data_directory ? data_directory : DEFAULT_DATA_DIR;
    sensor_data_result_t *result = sensor_data_range_by_sensor_id_ext(
        dir, s->sensor_id, start_time, end_time, recursive_search, extension_filter, max_depth);
    
    if (!result || result->count == 0) {
        sensor_data_result_free(result);
        return;
    }
    
    /* Use downsample_to_graph which handles both small and large datasets */
    downsample_to_graph(result->values, result->count, &s->graph);
    
    s->has_data = 1;
    sensor_data_result_free(result);
}

/* Load data for all sensors */
static void load_all_data(void)
{
    for (int i = 0; i < num_sensors; i++) {
        load_sensor_data(i);
    }
}

/* Format time for display */
static void format_time(time_t t, char *buf, size_t len)
{
    struct tm *tm = localtime(&t);
    strftime(buf, len, "%Y-%m-%d %H:%M", tm);
}

/* Draw the screen */
static void draw_screen(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    
    erase();  /* Use erase() instead of clear() to avoid full terminal clear */
    
    /* Calculate layout */
    int header_height = 3;
    int graph_area_height = rows - header_height;
    int graph_height = (num_sensors > 0) ? graph_area_height / num_sensors : 0;
    
    /* Draw header */
    if (has_colors()) attron(COLOR_PAIR(COLOR_LABEL));
    
    char start_buf[32], end_buf[32];
    time_t start_time = window_end - get_window_duration();
    format_time(start_time, start_buf, sizeof(start_buf));
    format_time(window_end, end_buf, sizeof(end_buf));
    
    mvprintw(0, 2, "sensor-plot - %s mode", get_mode_name());
    mvprintw(1, 2, "Time range: %s to %s", start_buf, end_buf);
    mvprintw(2, 2, "Keys: h=hour d=day w=week m=month y=year, Left/Right=scroll, q=quit");
    
    if (has_colors()) attroff(COLOR_PAIR(COLOR_LABEL));
    
    /* Draw each sensor graph */
    for (int i = 0; i < num_sensors; i++) {
        sensor_plot_t *s = &sensors[i];
        
        int start_row = header_height + i * graph_height;
        int end_row = start_row + graph_height - 1;
        
        /* Ensure we don't go past screen bounds */
        if (end_row >= rows - 1) end_row = rows - 2;
        if (start_row >= end_row) continue;
        
        /* Draw sensor label */
        if (has_colors()) attron(COLOR_PAIR(sensor_colors[i]));
        mvprintw(start_row, 2, "[%d] %s", i + 1, s->sensor_id);
        
        if (!s->has_data) {
            mvprintw(start_row + 1, 4, "(no data)");
        }
        if (has_colors()) attroff(COLOR_PAIR(sensor_colors[i]));
        
        /* Draw graph if we have data */
        if (s->has_data && graph_height > 4) {
            /* Temporarily modify graph drawing color */
            draw_graph(&s->graph, start_row + 1, end_row, 0, cols - 1);
        }
    }
    
    refresh();
}

/* Print help message */
static void print_help(void)
{
    printf("Usage: sensor-plot [OPTIONS] --sensor SENSOR_ID [--sensor SENSOR_ID2] ... [PATH]\n");
    printf("\nDisplay historical sensor data graphs.\n");
    printf("\nOptions:\n");
    printf("  --sensor ID          Sensor ID to plot (up to %d sensors)\n", MAX_SENSORS);
    printf("  -r, --recursive      Search subdirectories (default)\n");
    printf("  -R, --no-recursive   Do not search subdirectories\n");
    printf("  -d, --depth N        Maximum directory depth to search\n");
    printf("  -e, --extension EXT  Only read files with this extension\n");
    printf("  --help               Show this help message\n");
    printf("\nIf PATH is not specified, defaults to %s\n", DEFAULT_DATA_DIR);
    printf("\nControls:\n");
    printf("  Left/Right    Scroll time window\n");
    printf("  h             Hour mode (1 hour view, 1 minute steps)\n");
    printf("  d             Day mode (24 hour view, 1 hour steps)\n");
    printf("  w             Week mode (7 day view, 1 day steps)\n");
    printf("  m             Month mode (30 day view, 1 week steps)\n");
    printf("  y             Year mode (365 day view, 1 month steps)\n");
    printf("  r             Reload data\n");
    printf("  q             Quit\n");
}

/* Parse command line arguments using sensor_plot_args module */
static int parse_args(int argc, char **argv)
{
    sensor_plot_args_t args;
    int result = sensor_plot_args_parse(argc, argv, &args);
    
    if (result == 1) {
        /* Help requested */
        print_help();
        sensor_plot_args_free(&args);
        return 1;
    }
    
    if (result < 0) {
        /* Error */
        fprintf(stderr, "Error: %s\n", args.error_message ? args.error_message : "unknown error");
        fprintf(stderr, "Use --help for usage information\n");
        sensor_plot_args_free(&args);
        return -1;
    }
    
    /* Copy parsed values to global state */
    for (int i = 0; i < args.num_sensors; i++) {
        sensors[num_sensors].sensor_id = args.sensor_ids[i];
        args.sensor_ids[i] = NULL;  /* Transfer ownership */
        sensors[num_sensors].has_data = 0;
        reset_graph(&sensors[num_sensors].graph);
        num_sensors++;
    }
    
    recursive_search = args.recursive;
    max_depth = args.max_depth;
    data_directory = args.data_directory;
    args.data_directory = NULL;  /* Transfer ownership */
    extension_filter = args.extension;
    args.extension = NULL;  /* Transfer ownership */
    
    sensor_plot_args_free(&args);
    return 0;
}

/* Cleanup */
static void cleanup(void)
{
    for (int i = 0; i < num_sensors; i++) {
        free(sensors[i].sensor_id);
    }
    free(data_directory);
    free(extension_filter);
}

int main(int argc, char **argv)
{
    /* Parse arguments */
    int ret = parse_args(argc, argv);
    if (ret != 0) {
        cleanup();
        return (ret < 0) ? 1 : 0;
    }
    
    /* Set up signal handlers */
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    /* Initialize window to end at current time */
    window_end = time(NULL);
    
    /* Initialize ncurses */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100);  /* 100ms timeout for getch */
    
    /* Initialize colors */
    if (has_colors()) {
        start_color();
        init_pair(COLOR_FRAME, COLOR_GREEN, COLOR_BLACK);
        init_pair(COLOR_DATA1, COLOR_CYAN, COLOR_BLACK);
        init_pair(COLOR_DATA2, COLOR_YELLOW, COLOR_BLACK);
        init_pair(COLOR_DATA3, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(COLOR_DATA4, COLOR_RED, COLOR_BLACK);
        init_pair(COLOR_DATA5, COLOR_BLUE, COLOR_BLACK);
        init_pair(COLOR_LABEL, COLOR_WHITE, COLOR_BLACK);
    }
    
    /* Load initial data */
    load_all_data();
    
    /* Draw initial screen */
    draw_screen();
    
    /* Main loop */
    while (running) {
        int ch = getch();
        if (ch == ERR) continue;
        
        int needs_redraw = 0;
        
        switch (ch) {
            case 'q':
            case 'Q':
                running = 0;
                break;
                
            case KEY_LEFT:
                /* Move window back */
                window_end -= get_step_size();
                load_all_data();
                needs_redraw = 1;
                break;
                
            case KEY_RIGHT:
                /* Move window forward */
                window_end += get_step_size();
                /* Don't go past current time */
                if (window_end > time(NULL)) {
                    window_end = time(NULL);
                }
                load_all_data();
                needs_redraw = 1;
                break;
                
            case 'd':
            case 'D':
                current_mode = MODE_DAY;
                load_all_data();
                needs_redraw = 1;
                break;
                
            case 'w':
            case 'W':
                current_mode = MODE_WEEK;
                load_all_data();
                needs_redraw = 1;
                break;
                
            case 'h':
            case 'H':
                current_mode = MODE_HOUR;
                load_all_data();
                needs_redraw = 1;
                break;
                
            case 'm':
            case 'M':
                current_mode = MODE_MONTH;
                load_all_data();
                needs_redraw = 1;
                break;
                
            case 'y':
            case 'Y':
                current_mode = MODE_YEAR;
                load_all_data();
                needs_redraw = 1;
                break;
                
            case 'r':
            case 'R':
                /* Reload data */
                load_all_data();
                needs_redraw = 1;
                break;
                
            case KEY_RESIZE:
                /* Terminal resized, redraw */
                needs_redraw = 1;
                break;
        }
        
        if (needs_redraw) {
            draw_screen();
        }
    }
    
    /* Cleanup */
    endwin();
    cleanup();
    
    return 0;
}
