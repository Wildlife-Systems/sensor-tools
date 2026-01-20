/* Cross-platform includes */
#ifdef _WIN32
    #include <curses.h>
    #include <windows.h>
    #include <io.h>
    #include <direct.h>
    #define popen _popen
    #define pclose _pclose
    #define access _access
    #define F_OK 0
    #define X_OK 0  /* Windows doesn't have X_OK */
    #define PATH_SEP ';'
    #define DIR_SEP '\\'
    #define strtok_r strtok_s
#else
    #include <ncurses.h>
    #include <unistd.h>
    #include <dirent.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #define PATH_SEP ':'
    #define DIR_SEP '/'
#endif

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include "graph.h"

static volatile sig_atomic_t resized = 0;

#ifndef _WIN32
/* SIGWINCH handler - only on POSIX systems */
static void handle_winch(int sig)
{
    (void)sig;
    resized = 1;
}
#endif

/* find_sensor_apps:
 *  Scans each directory in PATH for executable files whose names start with
 *  the given prefix (e.g. "sensor-"). Returns an array of malloc'd strings
 *  containing absolute paths. The number of results is stored in *out_count
 *  if non-NULL. Caller must free each string and the returned array. On
 *  error returns NULL and sets *out_count to 0.
 */
char **find_sensor_apps(const char *prefix, int *out_count)
{
    if (!prefix) return NULL;
    const char *path = getenv("PATH");
    if (!path) { if (out_count) *out_count = 0; return NULL; }

    char *paths = strdup(path);
    if (!paths) { if (out_count) *out_count = 0; return NULL; }

    size_t cap = 16;
    size_t n = 0;
    char **results = malloc(cap * sizeof(char*));
    if (!results) { free(paths); if (out_count) *out_count = 0; return NULL; }

    char *saveptr = NULL;
    char sep_str[2] = { PATH_SEP, '\0' };
    char *dir = strtok_r(paths, sep_str, &saveptr);
    while (dir) {
#ifdef _WIN32
        /* Windows: use FindFirstFile/FindNextFile */
        char pattern[512];
        snprintf(pattern, sizeof(pattern), "%s\\%s*", dir, prefix);
        WIN32_FIND_DATAA fdata;
        HANDLE hFind = FindFirstFileA(pattern, &fdata);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
                
                /* build full path */
                size_t len = strlen(dir) + 1 + strlen(fdata.cFileName) + 1;
                char *full = malloc(len);
                if (!full) continue;
                snprintf(full, len, "%s\\%s", dir, fdata.cFileName);
                
                /* skip duplicates */
                int dup = 0;
                for (size_t k = 0; k < n; ++k) {
                    const char *eb = basename_of(results[k]);
                    if (_stricmp(eb, fdata.cFileName) == 0) { dup = 1; break; }
                }
                if (dup) { free(full); continue; }
                
                if (n + 1 >= cap) {
                    cap *= 2;
                    char **tmp = realloc(results, cap * sizeof(char*));
                    if (!tmp) { free(full); continue; }
                    results = tmp;
                }
                results[n++] = full;
            } while (FindNextFileA(hFind, &fdata));
            FindClose(hFind);
        }
#else
        /* POSIX: use opendir/readdir */
        DIR *d = opendir(dir);
        if (d) {
            struct dirent *ent;
            while ((ent = readdir(d)) != NULL) {
                if (strncmp(ent->d_name, prefix, strlen(prefix)) != 0)
                    continue;

                /* build full path */
                size_t len = strlen(dir) + 1 + strlen(ent->d_name) + 1;
                char *full = malloc(len);
                if (!full) continue;
                snprintf(full, len, "%s/%s", dir, ent->d_name);

                /* check executable */
                if (access(full, X_OK) == 0) {
                    /* skip if we've already found the same basename */
                    int dup = 0;
                    for (size_t k = 0; k < n; ++k) {
                        const char *existing = results[k];
                        const char *eb = strrchr(existing, '/');
                        eb = eb ? eb + 1 : existing;
                        if (strcmp(eb, ent->d_name) == 0) { dup = 1; break; }
                    }
                    if (dup) { free(full); continue; }
                    if (n + 1 >= cap) {
                        cap *= 2;
                        char **tmp = realloc(results, cap * sizeof(char*));
                        if (!tmp) { free(full); continue; }
                        results = tmp;
                    }
                    results[n++] = full;
                } else {
                    free(full);
                }
            }
            closedir(d);
        }
#endif
        dir = strtok_r(NULL, sep_str, &saveptr);
    }

    free(paths);

    if (n == 0) {
        free(results);
        results = NULL;
    } else {
        /* shrink and NULL-terminate */
        char **tmp = realloc(results, (n + 1) * sizeof(char*));
        if (tmp) results = tmp;
        results[n] = NULL;
    }

    if (out_count) *out_count = (int)n;
    return results;
}

void free_sensor_apps(char **apps, int count)
{
    if (!apps) return;
    for (int i = 0; i < count; ++i) free(apps[i]);
    free(apps);
}

/* Run '<path> identify' and return:
 *   1 if the process exited normally with status 60
 *   0 if it exited with a different code or was killed
 *  -1 on fork/wait/exec error
 */
int run_identify(const char *path)
{
#ifdef _WIN32
    /* Windows: use system() and parse exit code */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "\"%s\" identify >nul 2>&1", path);
    int ret = system(cmd);
    return (ret == 60) ? 1 : 0;
#else
    /* POSIX: use fork/exec for performance */
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execl(path, path, "identify", (char*)NULL);
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return -1;
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status) == 60 ? 1 : 0;
    }
    return 0;
#endif
}

static const char *basename_of(const char *path)
{
#ifdef _WIN32
    const char *s = strrchr(path, '\\');
    const char *s2 = strrchr(path, '/');
    if (s2 > s) s = s2;
#else
    const char *s = strrchr(path, '/');
#endif
    return s ? s + 1 : path;
}

/* Run sensor and capture its output */
char *run_sensor_and_capture(const char *path)
{
#ifdef _WIN32
    /* Windows: use popen */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "\"%s\" all 2>&1", path);
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;
    
    char *output = malloc(4096);
    if (!output) {
        pclose(fp);
        return NULL;
    }
    
    size_t total = 0;
    size_t bytes_read;
    char temp[256];
    while ((bytes_read = fread(temp, 1, sizeof(temp) - 1, fp)) > 0) {
        if (total + bytes_read >= 4095) break;
        memcpy(output + total, temp, bytes_read);
        total += bytes_read;
    }
    output[total] = '\0';
    
    pclose(fp);
    return output;
#else
    /* POSIX: use fork/exec/pipe for performance */
    int pipefd[2];
    if (pipe(pipefd) == -1) return NULL;
    
    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    }
    
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        execl(path, path, "all", (char*)NULL); /* run sensor with "all" parameter */
        _exit(127);
    }
    
    close(pipefd[1]);
    
    /* read output */
    char *output = malloc(4096);
    if (!output) {
        close(pipefd[0]);
        waitpid(pid, NULL, 0);
        return NULL;
    }
    
    ssize_t total = 0;
    ssize_t bytes_read;
    while ((bytes_read = read(pipefd[0], output + total, 4095 - total)) > 0) {
        total += bytes_read;
        if (total >= 4095) break;
    }
    output[total] = '\0';
    
    close(pipefd[0]);
    waitpid(pid, NULL, 0);
    
    return output;
#endif
}

/* Parse JSON array output into individual results */
char **parse_sensor_results(const char *output, int *count)
{
    if (!output || !count) return NULL;
    
    *count = 0;
    
    /* Simple JSON array parsing - look for objects separated by },{ */
    const char *start = strchr(output, '{');
    if (!start) return NULL;
    
    /* Count objects by counting opening braces */
    int brace_count = 0;
    int obj_count = 0;
    for (const char *p = start; *p; p++) {
        if (*p == '{') {
            if (brace_count == 0) obj_count++;
            brace_count++;
        } else if (*p == '}') {
            brace_count--;
        }
    }
    
    if (obj_count == 0) return NULL;
    
    char **results = malloc(obj_count * sizeof(char*));
    if (!results) return NULL;
    
    /* Extract individual JSON objects */
    int result_idx = 0;
    const char *obj_start = start;
    brace_count = 0;
    
    for (const char *p = start; *p && result_idx < obj_count; p++) {
        if (*p == '{') {
            if (brace_count == 0) obj_start = p;
            brace_count++;
        } else if (*p == '}') {
            brace_count--;
            if (brace_count == 0) {
                /* Found complete object */
                size_t len = p - obj_start + 1;
                results[result_idx] = malloc(len + 1);
                if (results[result_idx]) {
                    memcpy(results[result_idx], obj_start, len);
                    results[result_idx][len] = '\0';
                    result_idx++;
                }
            }
        }
    }
    
    *count = result_idx;
    return results;
}

void free_sensor_results(char **results, int count)
{
    if (!results) return;
    for (int i = 0; i < count; i++) {
        if (results[i]) free(results[i]);
    }
    free(results);
}

/* Format JSON string for better readability */
char *format_json(const char *json_str)
{
    if (!json_str) return NULL;
    
    size_t input_len = strlen(json_str);
    /* Allocate more space for formatting */
    char *formatted = malloc(input_len * 2 + 100);
    if (!formatted) return NULL;
    
    const char *src = json_str;
    char *dst = formatted;
    int indent = 0;
    int in_string = 0;
    
    while (*src) {
        char ch = *src;
        
        if (ch == '"' && (src == json_str || *(src-1) != '\\')) {
            in_string = !in_string;
        }
        
        if (!in_string) {
            if (ch == '{' || ch == '[') {
                *dst++ = ch;
                *dst++ = '\n';
                indent++;
                for (int i = 0; i < indent * 2; i++) *dst++ = ' ';
            } else if (ch == '}' || ch == ']') {
                if (*(dst-1) == ' ') {
                    /* Remove trailing spaces */
                    while (*(dst-1) == ' ') dst--;
                }
                *dst++ = '\n';
                indent--;
                for (int i = 0; i < indent * 2; i++) *dst++ = ' ';
                *dst++ = ch;
            } else if (ch == ',') {
                *dst++ = ch;
                *dst++ = '\n';
                for (int i = 0; i < indent * 2; i++) *dst++ = ' ';
            } else if (ch == ':') {
                *dst++ = ch;
                *dst++ = ' ';
            } else {
                *dst++ = ch;
            }
        } else {
            *dst++ = ch;
        }
        src++;
    }
    *dst = '\0';
    
    return formatted;
}

/* Extract numeric value from JSON object */
double extract_json_value(const char *json_str)
{
    if (!json_str) return 0.0;
    
    /* Look for "value": followed by a number */
    const char *value_pos = strstr(json_str, "\"value\":");
    if (!value_pos) return 0.0;
    
    /* Skip past "value": */
    value_pos += 8;
    
    /* Skip whitespace */
    while (*value_pos == ' ' || *value_pos == '\t') value_pos++;
    
    /* Parse the number */
    char *endptr;
    double val = strtod(value_pos, &endptr);
    
    /* Check if we actually parsed a number */
    if (endptr == value_pos) return 0.0;
    
    return val;
}

/* scan sensors once - only include those that identify correctly */
void scan_sensors(char ***apps_p, int *napps_p, int **ok_p)
{
    char **all_apps = find_sensor_apps("sensor-", napps_p);
    if (*napps_p == 0 || !all_apps) {
        *apps_p = NULL;
        *napps_p = 0;
        *ok_p = NULL;
        return;
    }
    
    /* filter to only include sensors that identify correctly */
    char **good_apps = malloc(*napps_p * sizeof(char*));
    int good_count = 0;
    
    for (int i = 0; i < *napps_p; ++i) {
        if (run_identify(all_apps[i]) == 1) {
            good_apps[good_count] = strdup(all_apps[i]);
            good_count++;
        }
    }
    
    /* clean up temporary array */
    free_sensor_apps(all_apps, *napps_p);
    
    if (good_count == 0) {
        free(good_apps);
        *apps_p = NULL;
        *napps_p = 0;
        *ok_p = NULL;
    } else {
        *apps_p = good_apps;
        *napps_p = good_count;
        *ok_p = NULL; /* no longer needed */
    }
}

int main(void)
{
    initscr();

    /* Initialize colors */
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);  /* Green frame */
        init_pair(2, COLOR_BLUE, COLOR_BLACK);   /* Blue data points/lines */
        init_pair(3, COLOR_BLACK, COLOR_WHITE);  /* Reversed header text */
    }

#ifndef _WIN32
    /* SIGWINCH for terminal resize - only on POSIX */
    signal(SIGWINCH, handle_winch);
#endif
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(1000); /* 1 second timeout for graph updates */

    /* Discover sensor-* apps and run 'identify' on each */
    int napps = 0;
    char **apps = NULL;
    int *ok = NULL;

    /* scan once at startup */
    scan_sensors(&apps, &napps, &ok);
    
    int selected_sensor = -1; /* -1 = main menu, >= 0 = showing sensor output */
    char *sensor_output = NULL;
    char **sensor_results = NULL; /* array of individual JSON objects */
    int num_results = 0;
    int current_result = 0; /* index of currently displayed result */
    
    graph_data_t graph = {0}; /* graph data for current sensor */
    time_t last_update = 0;
    
    int ch;
    while (1) {
        if (resized) {
            resized = 0;
            endwin();
            refresh();
            clear();
        }

        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        erase();
        box(stdscr, 0, 0);
        
        /* Update sensor data periodically when viewing a sensor */
        time_t now = time(NULL);
        if (selected_sensor >= 0 && (now - last_update >= 1)) {
            /* Update sensor reading every second */
            if (sensor_output) free(sensor_output);
            if (sensor_results) free_sensor_results(sensor_results, num_results);
            
            sensor_output = run_sensor_and_capture(apps[selected_sensor]);
            if (sensor_output) {
                sensor_results = parse_sensor_results(sensor_output, &num_results);
                
                /* Ensure current_result is still valid after update */
                if (current_result >= num_results) {
                    current_result = 0;
                }
                
                /* Add current value to graph */
                if (sensor_results && current_result < num_results) {
                    double value = extract_json_value(sensor_results[current_result]);
                    add_graph_point(&graph, value);
                }
            } else {
                /* If sensor reading failed, reset everything */
                sensor_results = NULL;
                num_results = 0;
                current_result = 0;
            }
            last_update = now;
        }
        
        if (selected_sensor == -1) {
            /* main menu mode */
            attron(COLOR_PAIR(3));
            /* Fill entire header row with background color */
            for (int i = 0; i < cols; i++) {
                mvwprintw(stdscr, 1, i, " ");
            }
            mvwprintw(stdscr, 1, 2, "ws-sensors - Select a sensor to view data");
            attroff(COLOR_PAIR(3));
            mvwprintw(stdscr, 2, 2, "Found %d working sensor-* apps", napps);

            /* list results (truncate to available space, leave room for bottom menu) */
            int start_row = 4;
            int max_list = rows - start_row - 4; /* leave room for bottom menu */
            for (int i = 0; i < napps && i < max_list; ++i) {
                const char *name = basename_of(apps[i]);
                mvwprintw(stdscr, start_row + i, 4, "%s", name);
            }
            
            /* custom bottom menu showing numbered sensors and quit */
            if (napps > 0) {
                int menu_row = rows - 2;
                int col = 2;
                
                /* add numbered sensor options first */
                for (int i = 0; i < napps && col < cols - 20; ++i) {
                    const char *name = basename_of(apps[i]);
                    /* remove 'sensor-' prefix from display name */
                    const char *display_name = name;
                    if (strncmp(name, "sensor-", 7) == 0) {
                        display_name = name + 7;
                    }
                    char menu_item[32];
                    snprintf(menu_item, sizeof(menu_item), "%d:%s", i + 1, display_name);
                    mvwprintw(stdscr, menu_row, col, "%s", menu_item);
                    col += strlen(menu_item) + 2; /* space between items */
                }
                
                /* add quit option last */
                mvwprintw(stdscr, menu_row, col, "q:quit");
            }
        } else {
            /* sensor output mode */
            const char *name = basename_of(apps[selected_sensor]);
            if (num_results > 1) {
                attron(COLOR_PAIR(3));
                /* Fill entire header row with background color */
                for (int i = 0; i < cols; i++) {
                    mvwprintw(stdscr, 1, i, " ");
                }
                mvwprintw(stdscr, 1, 2, "Sensor: %s (%d/%d) - 'b':back '[':prev ']':next", 
                         name, current_result + 1, num_results);
                attroff(COLOR_PAIR(3));
            } else {
                attron(COLOR_PAIR(3));
                /* Fill entire header row with background color */
                for (int i = 0; i < cols; i++) {
                    mvwprintw(stdscr, 1, i, " ");
                }
                mvwprintw(stdscr, 1, 2, "Sensor: %s - Press 'b' to go back", name);
                attroff(COLOR_PAIR(3));
            }
            
            /* Determine space allocation */
            int data_end_row = rows / 2; /* First half for data */
            int graph_start_row = data_end_row + 1;
            int graph_end_row = rows - 3;
            
            if (sensor_results && current_result < num_results) {
                /* display current sensor result, formatted for readability */
                char *formatted_json = format_json(sensor_results[current_result]);
                if (formatted_json) {
                    char *line = strtok(formatted_json, "\n");
                    int line_num = 3;
                    while (line && line_num < data_end_row) {
                        /* Display full line, wrapping if necessary */
                        int line_len = strlen(line);
                        int max_width = cols - 4; /* Account for border and padding */
                        
                        if (line_len <= max_width) {
                            mvwprintw(stdscr, line_num, 2, "%s", line);
                            line_num++;
                        } else {
                            /* Wrap long lines */
                            char *pos = line;
                            while (*pos && line_num < data_end_row) {
                                size_t remaining = strlen(pos);
                                int chunk_len = (remaining > (size_t)max_width) ? max_width : (int)remaining;
                                mvwprintw(stdscr, line_num, 2, "%.*s", chunk_len, pos);
                                pos += chunk_len;
                                line_num++;
                            }
                        }
                        line = strtok(NULL, "\n");
                    }
                    free(formatted_json);
                } else {
                    mvwprintw(stdscr, 3, 2, "Memory allocation error");
                }
            } else if (sensor_output) {
                /* fallback to showing raw output if parsing failed */
                char *output_copy = strdup(sensor_output);
                if (output_copy) {
                    char *line = strtok(output_copy, "\n");
                    int line_num = 3;
                    while (line && line_num < data_end_row) {
                        /* Display full line, wrapping if necessary */
                        int line_len = strlen(line);
                        int max_width = cols - 4; /* Account for border and padding */
                        
                        if (line_len <= max_width) {
                            mvwprintw(stdscr, line_num, 2, "%s", line);
                            line_num++;
                        } else {
                            /* Wrap long lines */
                            char *pos = line;
                            while (*pos && line_num < data_end_row) {
                                size_t remaining = strlen(pos);
                                int chunk_len = (remaining > (size_t)max_width) ? max_width : (int)remaining;
                                mvwprintw(stdscr, line_num, 2, "%.*s", chunk_len, pos);
                                pos += chunk_len;
                                line_num++;
                            }
                        }
                        line = strtok(NULL, "\n");
                    }
                    free(output_copy);
                } else {
                    mvwprintw(stdscr, 3, 2, "Memory allocation error");
                }
            } else {
                mvwprintw(stdscr, 3, 2, "No output captured");
            }
            
            /* Draw the graph in the bottom half */
            if (graph_start_row < graph_end_row) {
                draw_graph(&graph, graph_start_row, graph_end_row, 1, cols - 2);
            }
        }
        
        /* refresh main window */
        refresh();

        ch = getch();
        if (ch == 'q' || ch == 'Q') {
            break;
        } else if (ch == 'b' || ch == 'B') {
            /* back to main menu */
            selected_sensor = -1;
            if (sensor_output) {
                free(sensor_output);
                sensor_output = NULL;
            }
            if (sensor_results) {
                free_sensor_results(sensor_results, num_results);
                sensor_results = NULL;
                num_results = 0;
                current_result = 0;
            }
            /* Reset graph data */
            reset_graph(&graph);
            last_update = 0;
        } else if (ch >= '1' && ch <= '9' && selected_sensor == -1) {
            /* select sensor */
            int sensor_idx = ch - '1';
            if (sensor_idx < napps) {
                selected_sensor = sensor_idx;
                if (sensor_output) {
                    free(sensor_output);
                    sensor_output = NULL;
                }
                /* Reset graph data for new sensor */
                reset_graph(&graph);
                last_update = 0;
                
                /* show loading message immediately */
                erase();
                box(stdscr, 0, 0);
                const char *name = basename_of(apps[sensor_idx]);
                attron(COLOR_PAIR(3));
                /* Fill entire header row with background color */
                for (int i = 0; i < cols; i++) {
                    mvwprintw(stdscr, 1, i, " ");
                }
                mvwprintw(stdscr, 1, 2, "Sensor: %s - Press 'b' to go back", name);
                attroff(COLOR_PAIR(3));
                mvwprintw(stdscr, 3, 2, "Loading...");
                refresh();
                
                /* now capture sensor output */
                sensor_output = run_sensor_and_capture(apps[sensor_idx]);
                
                /* parse into individual results if it's a JSON array */
                if (sensor_output) {
                    sensor_results = parse_sensor_results(sensor_output, &num_results);
                    current_result = 0;
                    
                    /* Add initial graph point */
                    if (sensor_results && current_result < num_results) {
                        double value = extract_json_value(sensor_results[current_result]);
                        add_graph_point(&graph, value);
                    }
                    last_update = time(NULL);
                }
            }
        } else if (ch == '[' && selected_sensor >= 0 && num_results > 1) {
            /* previous result */
            current_result = (current_result - 1 + num_results) % num_results;
            /* Reset graph data for new sensor result */
            reset_graph(&graph);
            /* Add initial graph point for the new result */
            if (sensor_results && current_result < num_results) {
                double value = extract_json_value(sensor_results[current_result]);
                add_graph_point(&graph, value);
            }
        } else if (ch == ']' && selected_sensor >= 0 && num_results > 1) {
            /* next result */
            current_result = (current_result + 1) % num_results;
            /* Reset graph data for new sensor result */
            reset_graph(&graph);
            /* Add initial graph point for the new result */
            if (sensor_results && current_result < num_results) {
                double value = extract_json_value(sensor_results[current_result]);
                add_graph_point(&graph, value);
            }
        }
    }

    endwin();

    if (sensor_output) free(sensor_output);
    if (sensor_results) free_sensor_results(sensor_results, num_results);
    free(ok);
    if (apps) free_sensor_apps(apps, napps);
    return 0;
}
