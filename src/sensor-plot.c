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
static void load_sensor_data(int sensor_idx, int screen_width)
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
    
    /* Use time-based downsampling to graph, scaled to screen width */
    downsample_to_graph(result->values, result->timestamps, result->count,
                        start_time, end_time, screen_width, &s->graph);
    
    s->has_data = 1;
    sensor_data_result_free(result);
}

/* Load data for all sensors, scaled to screen width */
static void load_all_data(int screen_width)
{
    for (int i = 0; i < num_sensors; i++) {
        load_sensor_data(i, screen_width);
    }
}

/* Check if any sensor has data */
static int any_sensor_has_data(void)
{
    for (int i = 0; i < num_sensors; i++) {
        if (sensors[i].has_data) return 1;
    }
    return 0;
}

/* Find the most recent timestamp across all sensors */
static time_t find_most_recent_timestamp(void)
{
    time_t most_recent = 0;
    const char *dir = data_directory ? data_directory : DEFAULT_DATA_DIR;
    
    for (int i = 0; i < num_sensors; i++) {
        sensor_data_result_t *result = sensor_data_tail_by_sensor_id(
            dir, sensors[i].sensor_id, 1, recursive_search);
        
        if (result && result->count > 0 && result->timestamps[0] > most_recent) {
            most_recent = (time_t)result->timestamps[0];
        }
        sensor_data_result_free(result);
    }
    
    return most_recent;
}

/* Find the earliest timestamp across all sensors */
static time_t find_earliest_timestamp(void)
{
    time_t earliest = 0;
    const char *dir = data_directory ? data_directory : DEFAULT_DATA_DIR;
    
    for (int i = 0; i < num_sensors; i++) {
        sensor_data_result_t *result = sensor_data_head_by_sensor_id(
            dir, sensors[i].sensor_id, 1, recursive_search);
        
        if (result && result->count > 0) {
            time_t ts = (time_t)result->timestamps[0];
            if (earliest == 0 || ts < earliest) {
                earliest = ts;
            }
        }
        sensor_data_result_free(result);
    }
    
    return earliest;
}

/* Format time for display */
static void format_time(time_t t, char *buf, size_t len)
{
    struct tm *tm = localtime(&t);
    strftime(buf, len, "%Y-%m-%d %H:%M", tm);
}

/* Dialog return codes */
#define DIALOG_CANCEL 0
#define DIALOG_CONFIRM 1
#define DIALOG_DRILLDOWN 2  /* Tab pressed - drill down to next level */

/* Show input dialog and get user input
 * Returns: DIALOG_CANCEL (0), DIALOG_CONFIRM (1), or DIALOG_DRILLDOWN (2) */
static int show_input_dialog_ex(const char *prompt, const char *hint, char *buf, size_t buflen)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    
    /* Calculate dialog size and position */
    int dialog_width = 44;
    int dialog_height = 5;
    int start_col = (cols - dialog_width) / 2;
    int start_row = (rows - dialog_height) / 2;
    
    /* Create dialog window */
    WINDOW *dialog = newwin(dialog_height, dialog_width, start_row, start_col);
    if (!dialog) return DIALOG_CANCEL;
    
    /* Draw border and prompt */
    box(dialog, 0, 0);
    mvwprintw(dialog, 1, 2, "%s", prompt);
    mvwprintw(dialog, 3, 2, "%s", hint);
    
    /* Input field position */
    int input_row = 2;
    int input_col = 2;
    int max_input = dialog_width - 4;
    if ((int)buflen - 1 < max_input) max_input = (int)buflen - 1;
    
    /* Show cursor and enable echo for input */
    curs_set(1);
    wmove(dialog, input_row, input_col);
    wrefresh(dialog);
    
    /* Read input character by character */
    int pos = 0;
    buf[0] = '\0';
    
    while (1) {
        int ch = wgetch(dialog);
        
        if (ch == 27) {  /* Escape */
            curs_set(0);
            delwin(dialog);
            return DIALOG_CANCEL;
        }
        else if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            buf[pos] = '\0';
            curs_set(0);
            delwin(dialog);
            return DIALOG_CONFIRM;
        }
        else if (ch == '\t' || ch == KEY_STAB) {  /* Tab */
            buf[pos] = '\0';
            curs_set(0);
            delwin(dialog);
            return DIALOG_DRILLDOWN;
        }
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (pos > 0) {
                pos--;
                buf[pos] = '\0';
                mvwprintw(dialog, input_row, input_col + pos, " ");
                wmove(dialog, input_row, input_col + pos);
                wrefresh(dialog);
            }
        }
        else if (pos < max_input && ch >= 32 && ch < 127) {
            buf[pos] = (char)ch;
            pos++;
            buf[pos] = '\0';
            mvwaddch(dialog, input_row, input_col + pos - 1, ch);
            wrefresh(dialog);
        }
    }
}

/* Wrapper for simple confirm/cancel dialog */
static int show_input_dialog(const char *prompt, char *buf, size_t buflen)
{
    int result = show_input_dialog_ex(prompt, "[Enter=confirm, Esc=cancel]", buf, buflen);
    return (result == DIALOG_CONFIRM) ? 1 : 0;
}

/* Show cascading date/time dialogs starting from year
 * Returns 1 if a valid selection was made, 0 if cancelled */
static int show_datetime_picker(void)
{
    char buf[16];
    int result;
    int year, month, day, hour;
    struct tm tm_date = {0};
    
    /* Year dialog */
    result = show_input_dialog_ex("Enter year (e.g. 2025):", 
                                   "[Enter=go, Tab=month, Esc=cancel]", buf, sizeof(buf));
    if (result == DIALOG_CANCEL) return 0;
    
    year = atoi(buf);
    if (year < 1970 || year > 2100) return 0;
    
    tm_date.tm_year = year - 1900;
    tm_date.tm_mon = 0;
    tm_date.tm_mday = 1;
    tm_date.tm_hour = 0;
    tm_date.tm_min = 0;
    tm_date.tm_sec = 0;
    tm_date.tm_isdst = -1;
    
    if (result == DIALOG_CONFIRM) {
        /* Show whole year */
        time_t start = mktime(&tm_date);
        if (start == (time_t)-1) return 0;
        current_mode = MODE_YEAR;
        window_end = start + get_window_duration();
        return 1;
    }
    
    /* Month dialog (Tab was pressed) */
    result = show_input_dialog_ex("Enter month (1-12):", 
                                   "[Enter=go, Tab=day, Esc=cancel]", buf, sizeof(buf));
    if (result == DIALOG_CANCEL) return 0;
    
    month = atoi(buf);
    if (month < 1 || month > 12) return 0;
    tm_date.tm_mon = month - 1;
    
    if (result == DIALOG_CONFIRM) {
        /* Show whole month */
        time_t start = mktime(&tm_date);
        if (start == (time_t)-1) return 0;
        current_mode = MODE_MONTH;
        window_end = start + get_window_duration();
        return 1;
    }
    
    /* Calculate days in this month */
    int days_in_month;
    if (month == 2) {
        /* February - check for leap year */
        int is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        days_in_month = is_leap ? 29 : 28;
    } else if (month == 4 || month == 6 || month == 9 || month == 11) {
        days_in_month = 30;
    } else {
        days_in_month = 31;
    }
    
    /* Day dialog (Tab was pressed) */
    char day_prompt[32];
    snprintf(day_prompt, sizeof(day_prompt), "Enter day (1-%d):", days_in_month);
    result = show_input_dialog_ex(day_prompt, 
                                   "[Enter=go, Tab=hour, Esc=cancel]", buf, sizeof(buf));
    if (result == DIALOG_CANCEL) return 0;
    
    day = atoi(buf);
    if (day < 1 || day > days_in_month) return 0;
    tm_date.tm_mday = day;
    
    if (result == DIALOG_CONFIRM) {
        /* Show whole day */
        time_t start = mktime(&tm_date);
        if (start == (time_t)-1) return 0;
        current_mode = MODE_DAY;
        window_end = start + get_window_duration();
        return 1;
    }
    
    /* Hour dialog (Tab was pressed) */
    result = show_input_dialog_ex("Enter hour (0-23):", 
                                   "[Enter=go, Esc=cancel]", buf, sizeof(buf));
    if (result == DIALOG_CANCEL) return 0;
    
    hour = atoi(buf);
    if (hour < 0 || hour > 23) return 0;
    tm_date.tm_hour = hour;
    
    /* Show whole hour */
    time_t start = mktime(&tm_date);
    if (start == (time_t)-1) return 0;
    current_mode = MODE_HOUR;
    window_end = start + get_window_duration();
    return 1;
}

/* Parse year and set window to show entire year */
static int goto_year(int year)
{
    if (year < 1970 || year > 2100) return 0;
    
    struct tm tm_start = {0};
    tm_start.tm_year = year - 1900;
    tm_start.tm_mon = 0;   /* January */
    tm_start.tm_mday = 1;
    tm_start.tm_hour = 0;
    tm_start.tm_min = 0;
    tm_start.tm_sec = 0;
    tm_start.tm_isdst = -1;
    
    time_t start = mktime(&tm_start);
    if (start == (time_t)-1) return 0;
    
    /* Set to year mode and position window at end of year */
    current_mode = MODE_YEAR;
    window_end = start + get_window_duration();
    
    return 1;
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
    mvprintw(2, 2, "Keys: h/d/w/m/y=scale, arrows=scroll, n=newest s=start +/-=zoom, q=quit");
    
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
    printf("  n             Jump to newest data (most recent on right)\n");
    printf("  s             Jump to start (earliest data on left)\n");
    printf("  +/-           Zoom in/out (change time scale)\n");
    printf("  h             Hour mode (1 hour view, 1 minute steps)\n");
    printf("  d             Day mode (24 hour view, 1 hour steps)\n");
    printf("  w             Week mode (7 day view, 1 day steps)\n");
    printf("  m             Month mode (30 day view, 1 week steps)\n");
    printf("  y             Year mode (365 day view, 1 month steps)\n");
    printf("  Y             Go to date (year -> Tab -> month -> Tab -> day -> Tab -> hour)\n");
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
    
    /* Get initial screen width */
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    (void)rows;  /* unused here */
    
    /* Load initial data */
    load_all_data(cols);
    
    /* If no data found in current window, seek to most recent available data */
    if (!any_sensor_has_data()) {
        time_t most_recent = find_most_recent_timestamp();
        if (most_recent > 0) {
            window_end = most_recent;
            load_all_data(cols);
        }
    }
    
    /* Draw initial screen */
    draw_screen();
    
    /* Main loop */
    while (running) {
        int ch = getch();
        if (ch == ERR) continue;
        
        int needs_redraw = 0;
        int needs_reload = 0;
        
        /* Get current screen dimensions */
        getmaxyx(stdscr, rows, cols);
        (void)rows;
        
        switch (ch) {
            case 'q':
            case 'Q':
                running = 0;
                break;
                
            case KEY_LEFT:
                /* Move window back */
                window_end -= get_step_size();
                needs_reload = 1;
                break;
                
            case KEY_RIGHT:
                /* Move window forward */
                window_end += get_step_size();
                /* Don't go past current time */
                if (window_end > time(NULL)) {
                    window_end = time(NULL);
                }
                needs_reload = 1;
                break;
                
            case 'd':
            case 'D':
                current_mode = MODE_DAY;
                needs_reload = 1;
                break;
                
            case 'w':
            case 'W':
                current_mode = MODE_WEEK;
                needs_reload = 1;
                break;
                
            case 'h':
            case 'H':
                current_mode = MODE_HOUR;
                needs_reload = 1;
                break;
                
            case 'm':
            case 'M':
                current_mode = MODE_MONTH;
                needs_reload = 1;
                break;
                
            case 'y':
                current_mode = MODE_YEAR;
                needs_reload = 1;
                break;
                
            case 'Y':
                /* Show cascading datetime picker dialog */
                if (show_datetime_picker()) {
                    needs_reload = 1;
                }
                /* Redraw screen after dialog closes */
                needs_redraw = 1;
                break;
                
            case 'r':
                /* Reload data */
                needs_reload = 1;
                break;
                
            case 'n':
            case 'N':
                /* Jump to newest data (most recent on right edge) */
                {
                    time_t most_recent = find_most_recent_timestamp();
                    if (most_recent > 0) {
                        window_end = most_recent;
                    }
                }
                needs_reload = 1;
                break;
                
            case 's':
            case 'S':
                /* Jump to start (earliest data on left edge) */
                {
                    time_t earliest = find_earliest_timestamp();
                    if (earliest > 0) {
                        /* Set window_end so earliest is at left edge */
                        window_end = earliest + get_window_duration();
                    }
                }
                needs_reload = 1;
                break;
                
            case '+':
            case '=':
                /* Zoom in by ~10% (decrease window duration) */
                {
                    /* Keep center point fixed while zooming */
                    long old_duration = get_window_duration();
                    long center = (long)window_end - old_duration / 2;
                    
                    /* Decrease mode (zoom in) */
                    if (current_mode > MODE_HOUR) {
                        current_mode--;
                    }
                    
                    long new_duration = get_window_duration();
                    window_end = (time_t)(center + new_duration / 2);
                }
                needs_reload = 1;
                break;
                
            case '-':
            case '_':
                /* Zoom out by ~10% (increase window duration) */
                {
                    /* Keep center point fixed while zooming */
                    long old_duration = get_window_duration();
                    long center = (long)window_end - old_duration / 2;
                    
                    /* Increase mode (zoom out) */
                    if (current_mode < MODE_YEAR) {
                        current_mode++;
                    }
                    
                    long new_duration = get_window_duration();
                    window_end = (time_t)(center + new_duration / 2);
                }
                needs_reload = 1;
                break;
                
            case KEY_RESIZE:
                /* Terminal resized, reload data with new width */
                needs_reload = 1;
                break;
        }
        
        if (needs_reload) {
            load_all_data(cols);
            needs_redraw = 1;
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
