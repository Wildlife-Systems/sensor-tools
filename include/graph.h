#ifndef GRAPH_H
#define GRAPH_H

#include <time.h>

#define MAX_GRAPH_POINTS 400

typedef struct {
    double values[MAX_GRAPH_POINTS];
    time_t timestamps[MAX_GRAPH_POINTS];
    int count;
    int start_idx; /* for circular buffer when full */
    int historical_count; /* number of points that are historical (lighter color) */
    double min_val, max_val;
} graph_data_t;

/* Add value to graph data */
void add_graph_point(graph_data_t *graph, double value);

/* Add historical value to graph data (plotted in lighter color) */
void add_historical_point(graph_data_t *graph, double value);

/* Draw graph in the specified area */
void draw_graph(graph_data_t *graph, int start_row, int end_row, int start_col, int end_col);

/* Reset graph data */
void reset_graph(graph_data_t *graph);

#endif /* GRAPH_H */