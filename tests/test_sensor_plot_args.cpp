/* Unit tests for sensor_plot_args - command-line argument parsing */

#include <cstdio>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "sensor_plot_args.h"
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
#define ASSERT_STR_EQ(a, b) do { \
    const char *_a = (a); const char *_b = (b); \
    if (_a == NULL && _b == NULL) break; \
    if (_a == NULL || _b == NULL || strcmp(_a, _b) != 0) { \
        printf("ASSERT_STR_EQ failed: \"%s\" != \"%s\"\n", _a ? _a : "(null)", _b ? _b : "(null)"); \
        return false; \
    } \
} while(0)
#define ASSERT_NULL(a) do { if ((a) != NULL) { printf("ASSERT_NULL failed: %s is not NULL\n", #a); return false; } } while(0)
#define ASSERT_NOT_NULL(a) do { if ((a) == NULL) { printf("ASSERT_NOT_NULL failed: %s is NULL\n", #a); return false; } } while(0)

/* Test basic sensor argument */
bool test_single_sensor() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"--sensor", (char*)"temp_sensor"};
    int result = sensor_plot_args_parse(3, argv, &args);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(args.num_sensors, 1);
    ASSERT_STR_EQ(args.sensor_ids[0], "temp_sensor");
    ASSERT_EQ(args.recursive, 1);  /* default */
    ASSERT_EQ(args.max_depth, -1); /* default unlimited */
    ASSERT_NULL(args.data_directory);
    ASSERT_NULL(args.extension);
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test multiple sensors */
bool test_multiple_sensors() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"--sensor", (char*)"sensor1", 
                    (char*)"--sensor", (char*)"sensor2", (char*)"--sensor", (char*)"sensor3"};
    int result = sensor_plot_args_parse(7, argv, &args);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(args.num_sensors, 3);
    ASSERT_STR_EQ(args.sensor_ids[0], "sensor1");
    ASSERT_STR_EQ(args.sensor_ids[1], "sensor2");
    ASSERT_STR_EQ(args.sensor_ids[2], "sensor3");
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test too many sensors */
bool test_too_many_sensors() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", 
                    (char*)"--sensor", (char*)"s1",
                    (char*)"--sensor", (char*)"s2",
                    (char*)"--sensor", (char*)"s3",
                    (char*)"--sensor", (char*)"s4",
                    (char*)"--sensor", (char*)"s5",
                    (char*)"--sensor", (char*)"s6"};  /* 6th sensor - too many */
    int result = sensor_plot_args_parse(13, argv, &args);
    
    ASSERT_EQ(result, -1);
    ASSERT_NOT_NULL(args.error_message);
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test no sensors (error case) */
bool test_no_sensors() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot"};
    int result = sensor_plot_args_parse(1, argv, &args);
    
    ASSERT_EQ(result, -1);
    ASSERT_NOT_NULL(args.error_message);
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test recursive flag (default) */
bool test_recursive_default() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"--sensor", (char*)"s1"};
    int result = sensor_plot_args_parse(3, argv, &args);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(args.recursive, 1);
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test -r flag */
bool test_recursive_short() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"-r", (char*)"--sensor", (char*)"s1"};
    int result = sensor_plot_args_parse(4, argv, &args);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(args.recursive, 1);
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test --recursive flag */
bool test_recursive_long() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"--recursive", (char*)"--sensor", (char*)"s1"};
    int result = sensor_plot_args_parse(4, argv, &args);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(args.recursive, 1);
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test -R (no recursive) */
bool test_no_recursive_short() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"-R", (char*)"--sensor", (char*)"s1"};
    int result = sensor_plot_args_parse(4, argv, &args);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(args.recursive, 0);
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test --no-recursive */
bool test_no_recursive_long() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"--no-recursive", (char*)"--sensor", (char*)"s1"};
    int result = sensor_plot_args_parse(4, argv, &args);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(args.recursive, 0);
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test extension with dot */
bool test_extension_with_dot() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"-e", (char*)".out", (char*)"--sensor", (char*)"s1"};
    int result = sensor_plot_args_parse(5, argv, &args);
    
    ASSERT_EQ(result, 0);
    ASSERT_STR_EQ(args.extension, ".out");
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test extension without dot (should add dot) */
bool test_extension_without_dot() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"-e", (char*)"csv", (char*)"--sensor", (char*)"s1"};
    int result = sensor_plot_args_parse(5, argv, &args);
    
    ASSERT_EQ(result, 0);
    ASSERT_STR_EQ(args.extension, ".csv");
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test --extension long form */
bool test_extension_long() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"--extension", (char*)"json", (char*)"--sensor", (char*)"s1"};
    int result = sensor_plot_args_parse(5, argv, &args);
    
    ASSERT_EQ(result, 0);
    ASSERT_STR_EQ(args.extension, ".json");
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test extension missing argument */
bool test_extension_missing_arg() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"--sensor", (char*)"s1", (char*)"-e"};
    int result = sensor_plot_args_parse(4, argv, &args);
    
    ASSERT_EQ(result, -1);
    ASSERT_NOT_NULL(args.error_message);
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test data directory positional argument */
bool test_data_directory() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"--sensor", (char*)"s1", (char*)"/data/sensors"};
    int result = sensor_plot_args_parse(4, argv, &args);
    
    ASSERT_EQ(result, 0);
    ASSERT_STR_EQ(args.data_directory, "/data/sensors");
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test data directory before sensor */
bool test_data_directory_first() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"/var/custom", (char*)"--sensor", (char*)"s1"};
    int result = sensor_plot_args_parse(4, argv, &args);
    
    ASSERT_EQ(result, 0);
    ASSERT_STR_EQ(args.data_directory, "/var/custom");
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test multiple positional args (last one wins) */
bool test_multiple_directories() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"/first", (char*)"--sensor", (char*)"s1", (char*)"/second"};
    int result = sensor_plot_args_parse(5, argv, &args);
    
    ASSERT_EQ(result, 0);
    ASSERT_STR_EQ(args.data_directory, "/second");
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test help flag */
bool test_help_flag() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"--help"};
    int result = sensor_plot_args_parse(2, argv, &args);
    
    ASSERT_EQ(result, 1);
    ASSERT_EQ(args.show_help, 1);
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test -h flag */
bool test_help_short() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"-h"};
    int result = sensor_plot_args_parse(2, argv, &args);
    
    ASSERT_EQ(result, 1);
    ASSERT_EQ(args.show_help, 1);
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test unknown option */
bool test_unknown_option() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"--sensor", (char*)"s1", (char*)"--unknown"};
    int result = sensor_plot_args_parse(4, argv, &args);
    
    ASSERT_EQ(result, -1);
    ASSERT_NOT_NULL(args.error_message);
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test depth short option */
bool test_depth_short() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"-d", (char*)"3", (char*)"--sensor", (char*)"s1"};
    int result = sensor_plot_args_parse(5, argv, &args);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(args.max_depth, 3);
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test depth long option */
bool test_depth_long() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"--depth", (char*)"5", (char*)"--sensor", (char*)"s1"};
    int result = sensor_plot_args_parse(5, argv, &args);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(args.max_depth, 5);
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test depth missing argument */
bool test_depth_missing_arg() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"-d"};
    int result = sensor_plot_args_parse(2, argv, &args);
    
    ASSERT_EQ(result, -1);
    ASSERT_NOT_NULL(args.error_message);
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test depth zero (current directory only) */
bool test_depth_zero() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"-d", (char*)"0", (char*)"--sensor", (char*)"s1"};
    int result = sensor_plot_args_parse(5, argv, &args);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(args.max_depth, 0);
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test combined options */
bool test_combined_options() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", 
                    (char*)"-R",
                    (char*)"-d", (char*)"2",
                    (char*)"-e", (char*)"out",
                    (char*)"--sensor", (char*)"temp1",
                    (char*)"--sensor", (char*)"temp2",
                    (char*)"/data/ws"};
    int result = sensor_plot_args_parse(11, argv, &args);
    
    ASSERT_EQ(result, 0);
    ASSERT_EQ(args.num_sensors, 2);
    ASSERT_STR_EQ(args.sensor_ids[0], "temp1");
    ASSERT_STR_EQ(args.sensor_ids[1], "temp2");
    ASSERT_EQ(args.recursive, 0);
    ASSERT_EQ(args.max_depth, 2);
    ASSERT_STR_EQ(args.extension, ".out");
    ASSERT_STR_EQ(args.data_directory, "/data/ws");
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test NULL args pointer */
bool test_null_args() {
    char *argv[] = {(char*)"sensor-plot", (char*)"--sensor", (char*)"s1"};
    int result = sensor_plot_args_parse(3, argv, NULL);
    
    ASSERT_EQ(result, -1);
    return true;
}

/* Test sensor missing argument */
bool test_sensor_missing_arg() {
    sensor_plot_args_t args;
    char *argv[] = {(char*)"sensor-plot", (char*)"--sensor"};
    int result = sensor_plot_args_parse(2, argv, &args);
    
    ASSERT_EQ(result, -1);
    ASSERT_NOT_NULL(args.error_message);
    
    sensor_plot_args_free(&args);
    return true;
}

/* Test args_free with NULL */
bool test_free_null() {
    sensor_plot_args_free(NULL);  /* Should not crash */
    return true;
}

int main() {
    printf("=== Sensor Plot Args Unit Tests ===\n\n");
    
    TEST(single_sensor);
    TEST(multiple_sensors);
    TEST(too_many_sensors);
    TEST(no_sensors);
    TEST(recursive_default);
    TEST(recursive_short);
    TEST(recursive_long);
    TEST(no_recursive_short);
    TEST(no_recursive_long);
    TEST(extension_with_dot);
    TEST(extension_without_dot);
    TEST(extension_long);
    TEST(extension_missing_arg);
    TEST(depth_short);
    TEST(depth_long);
    TEST(depth_missing_arg);
    TEST(depth_zero);
    TEST(data_directory);
    TEST(data_directory_first);
    TEST(multiple_directories);
    TEST(help_flag);
    TEST(help_short);
    TEST(unknown_option);
    TEST(combined_options);
    TEST(null_args);
    TEST(sensor_missing_arg);
    TEST(free_null);
    
    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
