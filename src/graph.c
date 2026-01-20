#include "graph.h"
#ifdef _WIN32
    #include <curses.h>
#else
    #include <ncurses.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Downsample values array to fit in graph, using bucket averaging */
int downsample_to_graph(const double *values, int count, graph_data_t *graph)
{
    if (!values || count <= 0 || !graph) return 0;
    
    reset_graph(graph);
    
    if (count <= MAX_GRAPH_POINTS) {
        /* All points fit - add them directly */
        for (int i = 0; i < count; i++) {
            add_graph_point(graph, values[i]);
        }
        return count;
    }
    
    /* Downsample by averaging points into buckets */
    int num_buckets = MAX_GRAPH_POINTS;
    double points_per_bucket = (double)count / num_buckets;
    int points_added = 0;
    
    for (int bucket = 0; bucket < num_buckets; bucket++) {
        int start_idx = (int)(bucket * points_per_bucket);
        int end_idx = (int)((bucket + 1) * points_per_bucket);
        if (end_idx > count) end_idx = count;
        if (start_idx >= end_idx) continue;
        
        /* Calculate average for this bucket */
        double sum = 0;
        int bucket_count = 0;
        for (int i = start_idx; i < end_idx; i++) {
            sum += values[i];
            bucket_count++;
        }
        if (bucket_count > 0) {
            add_graph_point(graph, sum / bucket_count);
            points_added++;
        }
    }
    
    return points_added;
}

/* Add value to graph data */
void add_graph_point(graph_data_t *graph, double value)
{
    if (!graph) return;
    
    time_t now = time(NULL);
    
    if (graph->count < MAX_GRAPH_POINTS) {
        /* Still filling up the buffer */
        int idx = graph->count;
        graph->values[idx] = value;
        graph->timestamps[idx] = now;
        graph->count++;
        
        /* Update min/max */
        if (graph->count == 1 || value < graph->min_val) graph->min_val = value;
        if (graph->count == 1 || value > graph->max_val) graph->max_val = value;
    } else {
        /* Buffer is full, use circular buffer */
        int idx = graph->start_idx;
        graph->values[idx] = value;
        graph->timestamps[idx] = now;
        graph->start_idx = (graph->start_idx + 1) % MAX_GRAPH_POINTS;
        
        /* Historical count decreases as old points are overwritten */
        if (graph->historical_count > 0) graph->historical_count--;
        
        /* Recalculate min/max for all values in buffer */
        graph->min_val = graph->max_val = graph->values[0];
        for (int i = 0; i < MAX_GRAPH_POINTS; i++) {
            if (graph->values[i] < graph->min_val) graph->min_val = graph->values[i];
            if (graph->values[i] > graph->max_val) graph->max_val = graph->values[i];
        }
    }
}

/* Add historical value to graph data (plotted in lighter color) */
void add_historical_point(graph_data_t *graph, double value)
{
    if (!graph) return;
    
    /* Add as normal point but track it as historical */
    add_graph_point(graph, value);
    graph->historical_count++;
}

/* Draw graph in the specified area */
void draw_graph(graph_data_t *graph, int start_row, int end_row, int start_col, int end_col)
{
    if (!graph || graph->count == 0) return;
    
    int graph_height = end_row - start_row;
    int graph_width = end_col - start_col;
    
    if (graph_height < 3 || graph_width < 10) return;
    
    /* Draw graph border */
    if (has_colors()) attron(COLOR_PAIR(1)); /* Green frame */
    for (int row = start_row; row <= end_row; row++) {
        mvaddch(row, start_col, '|');
        mvaddch(row, end_col, '|');
    }
    for (int col = start_col; col <= end_col; col++) {
        mvaddch(start_row, col, '-');
        mvaddch(end_row, col, '-');
    }
    if (has_colors()) attroff(COLOR_PAIR(1));
    
    /* Graph title and range */
    mvprintw(start_row, start_col + 2, "Graph (%.3f - %.3f)", graph->min_val, graph->max_val);
    
    /* Determine which values to display */
    int display_width = graph_width - 2; /* minus borders */
    int points_to_show = (graph->count < display_width) ? graph->count : display_width;
    
    /* Draw the graph points */
    int prev_graph_row = -1;
    int prev_col = -1;
    int prev_is_historical = 0;
    
    /* Calculate where historical points end */
    int historical_end_idx = graph->historical_count;
    
    for (int i = 0; i < points_to_show; i++) {
        int value_idx;
        int is_historical = 0;
        
        if (graph->count < MAX_GRAPH_POINTS) {
            /* Linear buffer */
            value_idx = graph->count - points_to_show + i;
            /* Point is historical if its index is less than historical_count */
            is_historical = (value_idx < historical_end_idx);
        } else {
            /* Circular buffer */
            int offset = MAX_GRAPH_POINTS - points_to_show + i;
            value_idx = (graph->start_idx + offset) % MAX_GRAPH_POINTS;
            /* In circular mode, first historical_count points from start are historical */
            int logical_idx = (value_idx - graph->start_idx + MAX_GRAPH_POINTS) % MAX_GRAPH_POINTS;
            is_historical = (logical_idx < graph->historical_count);
        }
        
        int color_pair = is_historical ? 4 : 2; /* Cyan for historical, Blue for live */
        
        if (value_idx >= 0 && value_idx < graph->count) {
            int graph_row;
            int current_col = start_col + 1 + i;
            
            if (graph->max_val <= graph->min_val) {
                /* No variation - draw horizontal line in middle */
                graph_row = start_row + (graph_height / 2);
            } else {
                /* Scale value to graph height */
                double val = graph->values[value_idx];
                double range = graph->max_val - graph->min_val;
                graph_row = end_row - 1 - (int)((val - graph->min_val) / range * (graph_height - 2));
            }
            
            /* Draw vertical connecting line from previous point */
            if (prev_graph_row != -1 && prev_col != -1) {
                int line_start = (prev_graph_row < graph_row) ? prev_graph_row : graph_row;
                int line_end = (prev_graph_row > graph_row) ? prev_graph_row : graph_row;
                
                /* Use previous point's color for connecting line */
                int line_color = prev_is_historical ? 4 : 2;
                if (has_colors()) attron(COLOR_PAIR(line_color));
                for (int row = line_start; row <= line_end; row++) {
                    if (row >= start_row + 1 && row <= end_row - 1) {
                        /* Use different character for connecting line vs data point */
                        if (row != prev_graph_row && row != graph_row) {
                            mvaddch(row, prev_col, '|');
                        }
                    }
                }
                if (has_colors()) attroff(COLOR_PAIR(line_color));
            }
            
            /* Draw the data point */
            if (graph_row >= start_row + 1 && graph_row <= end_row - 1) {
                if (has_colors()) attron(COLOR_PAIR(color_pair));
                mvaddch(graph_row, current_col, '*');
                if (has_colors()) attroff(COLOR_PAIR(color_pair));
            }
            
            prev_graph_row = graph_row;
            prev_col = current_col;
            prev_is_historical = is_historical;
        }
    }
}

/* Reset graph data */
void reset_graph(graph_data_t *graph)
{
    if (!graph) return;
    memset(graph, 0, sizeof(graph_data_t));
}