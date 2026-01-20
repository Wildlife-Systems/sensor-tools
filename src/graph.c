#include "graph.h"
#ifdef _WIN32
    #include <curses.h>
#else
    #include <ncurses.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
        
        /* Recalculate min/max for all values in buffer */
        graph->min_val = graph->max_val = graph->values[0];
        for (int i = 0; i < MAX_GRAPH_POINTS; i++) {
            if (graph->values[i] < graph->min_val) graph->min_val = graph->values[i];
            if (graph->values[i] > graph->max_val) graph->max_val = graph->values[i];
        }
    }
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
    
    for (int i = 0; i < points_to_show; i++) {
        int value_idx;
        if (graph->count < MAX_GRAPH_POINTS) {
            /* Linear buffer */
            value_idx = graph->count - points_to_show + i;
        } else {
            /* Circular buffer */
            int offset = MAX_GRAPH_POINTS - points_to_show + i;
            value_idx = (graph->start_idx + offset) % MAX_GRAPH_POINTS;
        }
        
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
                
                if (has_colors()) attron(COLOR_PAIR(2)); /* Blue lines */
                for (int row = line_start; row <= line_end; row++) {
                    if (row >= start_row + 1 && row <= end_row - 1) {
                        /* Use different character for connecting line vs data point */
                        if (row != prev_graph_row && row != graph_row) {
                            mvaddch(row, prev_col, '|');
                        }
                    }
                }
                if (has_colors()) attroff(COLOR_PAIR(2));
            }
            
            /* Draw the data point */
            if (graph_row >= start_row + 1 && graph_row <= end_row - 1) {
                if (has_colors()) attron(COLOR_PAIR(2)); /* Blue data points */
                mvaddch(graph_row, current_col, '*');
                if (has_colors()) attroff(COLOR_PAIR(2));
            }
            
            prev_graph_row = graph_row;
            prev_col = current_col;
        }
    }
}

/* Reset graph data */
void reset_graph(graph_data_t *graph)
{
    if (!graph) return;
    memset(graph, 0, sizeof(graph_data_t));
}