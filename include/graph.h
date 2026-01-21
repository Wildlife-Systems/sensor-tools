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

/* Downsample values array to fit in graph, using time-based bucket averaging
 * values: input array of values
 * timestamps: input array of timestamps (unix time)
 * count: number of values in input array
 * start_time: start of time window
 * end_time: end of time window
 * num_buckets: number of buckets to create (typically screen width)
 * graph: output graph to populate
 * Returns number of points added to graph */
int downsample_to_graph(const double *values, const long *timestamps, int count,
                        long start_time, long end_time, int num_buckets, graph_data_t *graph);

#endif /* GRAPH_H */
