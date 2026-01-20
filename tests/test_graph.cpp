/* Unit tests for graph data structure functions
 * Tests: add_graph_point, add_historical_point, reset_graph
 * Does NOT test draw_graph (requires ncurses)
 */

#include <cstdio>
#include <cstring>
#include <cmath>
#include <cassert>

/* Include graph.h but we won't link against ncurses - just testing data functions */
extern "C" {
#include "graph.h"
}

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    printf("  Testing %s... ", #name); \
    tests_run++; \
    if (test_##name()) { \
        printf("PASSED\n"); \
        tests_passed++; \
    } else { \
        printf("FAILED\n"); \
    } \
} while(0)

#define ASSERT(cond) do { if (!(cond)) { printf("ASSERT failed: %s\n", #cond); return false; } } while(0)
#define ASSERT_EQ(a, b) do { if ((a) != (b)) { printf("ASSERT_EQ failed: %s != %s (%d != %d)\n", #a, #b, (int)(a), (int)(b)); return false; } } while(0)
#define ASSERT_DOUBLE_EQ(a, b) do { if (fabs((a) - (b)) > 0.0001) { printf("ASSERT_DOUBLE_EQ failed: %s != %s (%.4f != %.4f)\n", #a, #b, (double)(a), (double)(b)); return false; } } while(0)

/* Test reset_graph */
bool test_reset_graph() {
    graph_data_t graph;
    
    /* Set some values */
    graph.count = 10;
    graph.start_idx = 5;
    graph.historical_count = 3;
    graph.min_val = 1.0;
    graph.max_val = 100.0;
    graph.values[0] = 42.0;
    
    reset_graph(&graph);
    
    ASSERT_EQ(graph.count, 0);
    ASSERT_EQ(graph.start_idx, 0);
    ASSERT_EQ(graph.historical_count, 0);
    ASSERT_DOUBLE_EQ(graph.min_val, 0.0);
    ASSERT_DOUBLE_EQ(graph.max_val, 0.0);
    
    return true;
}

/* Test NULL handling */
bool test_null_handling() {
    /* These should not crash */
    add_graph_point(NULL, 1.0);
    add_historical_point(NULL, 1.0);
    reset_graph(NULL);
    
    return true;
}

/* Test adding first point */
bool test_add_first_point() {
    graph_data_t graph;
    reset_graph(&graph);
    
    add_graph_point(&graph, 25.5);
    
    ASSERT_EQ(graph.count, 1);
    ASSERT_DOUBLE_EQ(graph.values[0], 25.5);
    ASSERT_DOUBLE_EQ(graph.min_val, 25.5);
    ASSERT_DOUBLE_EQ(graph.max_val, 25.5);
    ASSERT_EQ(graph.start_idx, 0);
    
    return true;
}

/* Test adding multiple points and min/max tracking */
bool test_min_max_tracking() {
    graph_data_t graph;
    reset_graph(&graph);
    
    add_graph_point(&graph, 10.0);
    ASSERT_DOUBLE_EQ(graph.min_val, 10.0);
    ASSERT_DOUBLE_EQ(graph.max_val, 10.0);
    
    add_graph_point(&graph, 20.0);
    ASSERT_DOUBLE_EQ(graph.min_val, 10.0);
    ASSERT_DOUBLE_EQ(graph.max_val, 20.0);
    
    add_graph_point(&graph, 5.0);
    ASSERT_DOUBLE_EQ(graph.min_val, 5.0);
    ASSERT_DOUBLE_EQ(graph.max_val, 20.0);
    
    add_graph_point(&graph, 15.0);
    ASSERT_DOUBLE_EQ(graph.min_val, 5.0);
    ASSERT_DOUBLE_EQ(graph.max_val, 20.0);
    
    add_graph_point(&graph, 25.0);
    ASSERT_DOUBLE_EQ(graph.min_val, 5.0);
    ASSERT_DOUBLE_EQ(graph.max_val, 25.0);
    
    ASSERT_EQ(graph.count, 5);
    
    return true;
}

/* Test negative values */
bool test_negative_values() {
    graph_data_t graph;
    reset_graph(&graph);
    
    add_graph_point(&graph, -10.0);
    add_graph_point(&graph, 10.0);
    add_graph_point(&graph, -20.0);
    
    ASSERT_DOUBLE_EQ(graph.min_val, -20.0);
    ASSERT_DOUBLE_EQ(graph.max_val, 10.0);
    ASSERT_EQ(graph.count, 3);
    
    return true;
}

/* Test historical point tracking */
bool test_historical_points() {
    graph_data_t graph;
    reset_graph(&graph);
    
    /* Add some historical points */
    add_historical_point(&graph, 1.0);
    add_historical_point(&graph, 2.0);
    add_historical_point(&graph, 3.0);
    
    ASSERT_EQ(graph.count, 3);
    ASSERT_EQ(graph.historical_count, 3);
    
    /* Add some regular points */
    add_graph_point(&graph, 4.0);
    add_graph_point(&graph, 5.0);
    
    ASSERT_EQ(graph.count, 5);
    ASSERT_EQ(graph.historical_count, 3);  /* Still 3 historical */
    
    return true;
}

/* Test filling buffer without overflow */
bool test_fill_buffer() {
    graph_data_t graph;
    reset_graph(&graph);
    
    /* Fill buffer to just before max */
    for (int i = 0; i < MAX_GRAPH_POINTS - 1; i++) {
        add_graph_point(&graph, (double)i);
    }
    
    ASSERT_EQ(graph.count, MAX_GRAPH_POINTS - 1);
    ASSERT_EQ(graph.start_idx, 0);  /* Not using circular yet */
    ASSERT_DOUBLE_EQ(graph.min_val, 0.0);
    ASSERT_DOUBLE_EQ(graph.max_val, (double)(MAX_GRAPH_POINTS - 2));
    
    return true;
}

/* Test circular buffer behavior */
bool test_circular_buffer() {
    graph_data_t graph;
    reset_graph(&graph);
    
    /* Fill buffer completely */
    for (int i = 0; i < MAX_GRAPH_POINTS; i++) {
        add_graph_point(&graph, (double)i);
    }
    
    ASSERT_EQ(graph.count, MAX_GRAPH_POINTS);
    ASSERT_EQ(graph.start_idx, 0);
    
    /* Add one more - should trigger circular buffer */
    add_graph_point(&graph, 999.0);
    
    ASSERT_EQ(graph.count, MAX_GRAPH_POINTS);
    ASSERT_EQ(graph.start_idx, 1);  /* Wrapped around */
    ASSERT_DOUBLE_EQ(graph.values[0], 999.0);  /* Overwrote position 0 */
    
    /* Add another */
    add_graph_point(&graph, 1000.0);
    ASSERT_EQ(graph.start_idx, 2);
    ASSERT_DOUBLE_EQ(graph.values[1], 1000.0);
    
    return true;
}

/* Test that circular buffer recalculates min/max correctly */
bool test_circular_buffer_minmax() {
    graph_data_t graph;
    reset_graph(&graph);
    
    /* Fill with values 0 to MAX_GRAPH_POINTS-1 */
    for (int i = 0; i < MAX_GRAPH_POINTS; i++) {
        add_graph_point(&graph, (double)i);
    }
    
    ASSERT_DOUBLE_EQ(graph.min_val, 0.0);
    ASSERT_DOUBLE_EQ(graph.max_val, (double)(MAX_GRAPH_POINTS - 1));
    
    /* Overwrite first value (which was 0) with a large value */
    add_graph_point(&graph, 500.0);
    
    /* Min should now be 1 (the old minimum 0 was overwritten) */
    ASSERT_DOUBLE_EQ(graph.min_val, 1.0);
    /* Max should be 500 */
    ASSERT_DOUBLE_EQ(graph.max_val, 500.0);
    
    return true;
}

/* Test historical count decreases when circular buffer overwrites */
bool test_historical_circular_buffer() {
    graph_data_t graph;
    reset_graph(&graph);
    
    /* Add historical points to fill buffer */
    for (int i = 0; i < MAX_GRAPH_POINTS; i++) {
        add_historical_point(&graph, (double)i);
    }
    
    ASSERT_EQ(graph.count, MAX_GRAPH_POINTS);
    ASSERT_EQ(graph.historical_count, MAX_GRAPH_POINTS);
    
    /* Add a regular point - should overwrite oldest historical */
    add_graph_point(&graph, 999.0);
    
    ASSERT_EQ(graph.count, MAX_GRAPH_POINTS);
    ASSERT_EQ(graph.historical_count, MAX_GRAPH_POINTS - 1);
    
    /* Add another regular point */
    add_graph_point(&graph, 1000.0);
    ASSERT_EQ(graph.historical_count, MAX_GRAPH_POINTS - 2);
    
    return true;
}

/* Test timestamps are set */
bool test_timestamps_set() {
    graph_data_t graph;
    reset_graph(&graph);
    
    add_graph_point(&graph, 42.0);
    
    /* Timestamp should be set to something reasonable (> 0) */
    ASSERT(graph.timestamps[0] > 0);
    
    return true;
}

/* Test zero and very small values */
bool test_zero_values() {
    graph_data_t graph;
    reset_graph(&graph);
    
    add_graph_point(&graph, 0.0);
    add_graph_point(&graph, 0.0001);
    add_graph_point(&graph, -0.0001);
    
    ASSERT_DOUBLE_EQ(graph.min_val, -0.0001);
    ASSERT_DOUBLE_EQ(graph.max_val, 0.0001);
    
    return true;
}

/* Test downsample_to_graph with time-based bucketing */
bool test_downsample_small_array() {
    graph_data_t graph;
    reset_graph(&graph);
    
    /* 5 values evenly spread over 5 seconds */
    double values[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    long timestamps[] = {100, 101, 102, 103, 104};
    
    /* Request 5 buckets for 5 seconds = 1 value per bucket */
    int result = downsample_to_graph(values, timestamps, 5, 100, 105, 5, &graph);
    
    ASSERT_EQ(result, 5);
    ASSERT_EQ(graph.count, 5);
    ASSERT_DOUBLE_EQ(graph.values[0], 1.0);
    ASSERT_DOUBLE_EQ(graph.values[4], 5.0);
    ASSERT_DOUBLE_EQ(graph.min_val, 1.0);
    ASSERT_DOUBLE_EQ(graph.max_val, 5.0);
    
    return true;
}

/* Test downsample_to_graph with null/invalid inputs */
bool test_downsample_null_handling() {
    graph_data_t graph;
    reset_graph(&graph);
    
    double values[] = {1.0, 2.0, 3.0};
    long timestamps[] = {100, 101, 102};
    
    ASSERT_EQ(downsample_to_graph(NULL, timestamps, 3, 100, 103, 3, &graph), 0);
    ASSERT_EQ(downsample_to_graph(values, NULL, 3, 100, 103, 3, &graph), 0);
    ASSERT_EQ(downsample_to_graph(values, timestamps, 0, 100, 103, 3, &graph), 0);
    ASSERT_EQ(downsample_to_graph(values, timestamps, -1, 100, 103, 3, &graph), 0);
    ASSERT_EQ(downsample_to_graph(values, timestamps, 3, 100, 103, 3, NULL), 0);
    /* Invalid time range */
    ASSERT_EQ(downsample_to_graph(values, timestamps, 3, 100, 100, 3, &graph), 0);
    ASSERT_EQ(downsample_to_graph(values, timestamps, 3, 100, 50, 3, &graph), 0);
    
    return true;
}

/* Test downsample_to_graph with time-based averaging */
bool test_downsample_large_array() {
    graph_data_t graph;
    reset_graph(&graph);
    
    /* Create 100 values over 100 seconds, downsample to 10 buckets */
    int size = 100;
    double *values = new double[size];
    long *timestamps = new long[size];
    
    for (int i = 0; i < size; i++) {
        values[i] = (double)i;
        timestamps[i] = 1000 + i;
    }
    
    /* 10 buckets over 100 seconds = 10 values per bucket */
    int result = downsample_to_graph(values, timestamps, size, 1000, 1100, 10, &graph);
    
    ASSERT_EQ(result, 10);
    ASSERT_EQ(graph.count, 10);
    
    /* First bucket (1000-1010): values 0-9, avg = 4.5 */
    ASSERT_DOUBLE_EQ(graph.values[0], 4.5);
    
    /* Last bucket (1090-1100): values 90-99, avg = 94.5 */
    ASSERT_DOUBLE_EQ(graph.values[9], 94.5);
    
    delete[] values;
    delete[] timestamps;
    return true;
}

/* Test downsample_to_graph averaging correctness */
bool test_downsample_averaging() {
    graph_data_t graph;
    reset_graph(&graph);
    
    /* Create values with known averages - 2 values in each bucket */
    double values[] = {10.0, 20.0, 30.0, 40.0, 50.0, 60.0};
    long timestamps[] = {100, 101, 102, 103, 104, 105};
    
    /* 3 buckets over 6 seconds = 2 values per bucket */
    int result = downsample_to_graph(values, timestamps, 6, 100, 106, 3, &graph);
    
    ASSERT_EQ(result, 3);
    
    /* Bucket 0 (100-102): values 10, 20 -> avg 15 */
    ASSERT_DOUBLE_EQ(graph.values[0], 15.0);
    /* Bucket 1 (102-104): values 30, 40 -> avg 35 */
    ASSERT_DOUBLE_EQ(graph.values[1], 35.0);
    /* Bucket 2 (104-106): values 50, 60 -> avg 55 */
    ASSERT_DOUBLE_EQ(graph.values[2], 55.0);
    
    return true;
}

/* Test downsample_to_graph with sparse data (empty buckets) */
bool test_downsample_sparse_data() {
    graph_data_t graph;
    reset_graph(&graph);
    
    /* Only 2 data points, but 10 buckets - most will be empty */
    double values[] = {10.0, 20.0};
    long timestamps[] = {100, 150};  /* 50 seconds apart */
    
    /* 10 buckets over 100 seconds */
    int result = downsample_to_graph(values, timestamps, 2, 100, 200, 10, &graph);
    
    /* Only 2 buckets have data */
    ASSERT_EQ(result, 2);
    ASSERT_EQ(graph.count, 2);
    ASSERT_DOUBLE_EQ(graph.values[0], 10.0);
    ASSERT_DOUBLE_EQ(graph.values[1], 20.0);
    
    return true;
}

/* Test downsample clears previous graph data */
bool test_downsample_resets_graph() {
    graph_data_t graph;
    
    /* Pre-fill with junk */
    graph.count = 100;
    graph.start_idx = 50;
    graph.min_val = -999.0;
    graph.max_val = 999.0;
    
    double values[] = {5.0, 10.0, 15.0};
    long timestamps[] = {100, 101, 102};
    int result = downsample_to_graph(values, timestamps, 3, 100, 103, 3, &graph);
    
    ASSERT_EQ(result, 3);
    ASSERT_EQ(graph.count, 3);
    ASSERT_EQ(graph.start_idx, 0);
    ASSERT_DOUBLE_EQ(graph.min_val, 5.0);
    ASSERT_DOUBLE_EQ(graph.max_val, 15.0);
    
    return true;
}

int main() {
    printf("=== Graph Data Structure Unit Tests ===\n\n");
    
    TEST(reset_graph);
    TEST(null_handling);
    TEST(add_first_point);
    TEST(min_max_tracking);
    TEST(negative_values);
    TEST(historical_points);
    TEST(fill_buffer);
    TEST(circular_buffer);
    TEST(circular_buffer_minmax);
    TEST(historical_circular_buffer);
    TEST(timestamps_set);
    TEST(zero_values);
    TEST(downsample_small_array);
    TEST(downsample_null_handling);
    TEST(downsample_large_array);
    TEST(downsample_averaging);
    TEST(downsample_sparse_data);
    TEST(downsample_resets_graph);
    
    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
