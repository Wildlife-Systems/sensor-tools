/* Unit tests for sensor_plot_utils - utility functions for sensor-plot */

#include <cstdio>

extern "C" {
#include "sensor_plot_utils.h"
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
#define ASSERT_EQ(a, b) do { if ((a) != (b)) { printf("ASSERT_EQ failed: %s != %s (%ld != %ld)\n", #a, #b, (long)(a), (long)(b)); return false; } } while(0)

/* ===== Window Duration Tests ===== */

bool test_window_duration_hour() {
    ASSERT_EQ(sensor_plot_get_window_duration(SENSOR_PLOT_MODE_HOUR), 3600L);
    return true;
}

bool test_window_duration_day() {
    ASSERT_EQ(sensor_plot_get_window_duration(SENSOR_PLOT_MODE_DAY), 24L * 3600L);
    return true;
}

bool test_window_duration_week() {
    ASSERT_EQ(sensor_plot_get_window_duration(SENSOR_PLOT_MODE_WEEK), 7L * 24L * 3600L);
    return true;
}

bool test_window_duration_month() {
    ASSERT_EQ(sensor_plot_get_window_duration(SENSOR_PLOT_MODE_MONTH), 30L * 24L * 3600L);
    return true;
}

bool test_window_duration_year() {
    ASSERT_EQ(sensor_plot_get_window_duration(SENSOR_PLOT_MODE_YEAR), 365L * 24L * 3600L);
    return true;
}

/* ===== Step Size Tests ===== */

bool test_step_size_hour() {
    ASSERT_EQ(sensor_plot_get_step_size(SENSOR_PLOT_MODE_HOUR), 60L);  /* 1 minute */
    return true;
}

bool test_step_size_day() {
    ASSERT_EQ(sensor_plot_get_step_size(SENSOR_PLOT_MODE_DAY), 3600L);  /* 1 hour */
    return true;
}

bool test_step_size_week() {
    ASSERT_EQ(sensor_plot_get_step_size(SENSOR_PLOT_MODE_WEEK), 24L * 3600L);  /* 1 day */
    return true;
}

bool test_step_size_month() {
    ASSERT_EQ(sensor_plot_get_step_size(SENSOR_PLOT_MODE_MONTH), 7L * 24L * 3600L);  /* 1 week */
    return true;
}

bool test_step_size_year() {
    ASSERT_EQ(sensor_plot_get_step_size(SENSOR_PLOT_MODE_YEAR), 30L * 24L * 3600L);  /* 1 month */
    return true;
}

/* ===== Leap Year Tests ===== */

bool test_leap_year_regular() {
    /* Regular years divisible by 4 are leap years */
    ASSERT(sensor_plot_is_leap_year(2024));
    ASSERT(sensor_plot_is_leap_year(2020));
    ASSERT(sensor_plot_is_leap_year(2016));
    return true;
}

bool test_leap_year_century_not_leap() {
    /* Century years not divisible by 400 are NOT leap years */
    ASSERT(!sensor_plot_is_leap_year(1900));
    ASSERT(!sensor_plot_is_leap_year(2100));
    ASSERT(!sensor_plot_is_leap_year(2200));
    return true;
}

bool test_leap_year_century_leap() {
    /* Century years divisible by 400 ARE leap years */
    ASSERT(sensor_plot_is_leap_year(2000));
    ASSERT(sensor_plot_is_leap_year(1600));
    ASSERT(sensor_plot_is_leap_year(2400));
    return true;
}

bool test_not_leap_year() {
    /* Regular non-leap years */
    ASSERT(!sensor_plot_is_leap_year(2023));
    ASSERT(!sensor_plot_is_leap_year(2025));
    ASSERT(!sensor_plot_is_leap_year(2026));
    return true;
}

/* ===== Days in Month Tests ===== */

bool test_days_in_january() {
    ASSERT_EQ(sensor_plot_days_in_month(2025, 1), 31);
    return true;
}

bool test_days_in_february_regular() {
    ASSERT_EQ(sensor_plot_days_in_month(2023, 2), 28);
    ASSERT_EQ(sensor_plot_days_in_month(2025, 2), 28);
    return true;
}

bool test_days_in_february_leap() {
    ASSERT_EQ(sensor_plot_days_in_month(2024, 2), 29);
    ASSERT_EQ(sensor_plot_days_in_month(2000, 2), 29);
    return true;
}

bool test_days_in_february_century_not_leap() {
    ASSERT_EQ(sensor_plot_days_in_month(1900, 2), 28);
    ASSERT_EQ(sensor_plot_days_in_month(2100, 2), 28);
    return true;
}

bool test_days_in_30_day_months() {
    ASSERT_EQ(sensor_plot_days_in_month(2025, 4), 30);   /* April */
    ASSERT_EQ(sensor_plot_days_in_month(2025, 6), 30);   /* June */
    ASSERT_EQ(sensor_plot_days_in_month(2025, 9), 30);   /* September */
    ASSERT_EQ(sensor_plot_days_in_month(2025, 11), 30);  /* November */
    return true;
}

bool test_days_in_31_day_months() {
    ASSERT_EQ(sensor_plot_days_in_month(2025, 1), 31);   /* January */
    ASSERT_EQ(sensor_plot_days_in_month(2025, 3), 31);   /* March */
    ASSERT_EQ(sensor_plot_days_in_month(2025, 5), 31);   /* May */
    ASSERT_EQ(sensor_plot_days_in_month(2025, 7), 31);   /* July */
    ASSERT_EQ(sensor_plot_days_in_month(2025, 8), 31);   /* August */
    ASSERT_EQ(sensor_plot_days_in_month(2025, 10), 31);  /* October */
    ASSERT_EQ(sensor_plot_days_in_month(2025, 12), 31);  /* December */
    return true;
}

bool test_days_in_invalid_month() {
    ASSERT_EQ(sensor_plot_days_in_month(2025, 0), 0);
    ASSERT_EQ(sensor_plot_days_in_month(2025, 13), 0);
    ASSERT_EQ(sensor_plot_days_in_month(2025, -1), 0);
    return true;
}

/* ===== Validation Tests ===== */

bool test_valid_year() {
    ASSERT(sensor_plot_valid_year(1970));
    ASSERT(sensor_plot_valid_year(2000));
    ASSERT(sensor_plot_valid_year(2025));
    ASSERT(sensor_plot_valid_year(2100));
    return true;
}

bool test_invalid_year() {
    ASSERT(!sensor_plot_valid_year(1969));
    ASSERT(!sensor_plot_valid_year(2101));
    ASSERT(!sensor_plot_valid_year(0));
    ASSERT(!sensor_plot_valid_year(-1));
    return true;
}

bool test_valid_month() {
    for (int m = 1; m <= 12; m++) {
        ASSERT(sensor_plot_valid_month(m));
    }
    return true;
}

bool test_invalid_month() {
    ASSERT(!sensor_plot_valid_month(0));
    ASSERT(!sensor_plot_valid_month(13));
    ASSERT(!sensor_plot_valid_month(-1));
    return true;
}

bool test_valid_day() {
    /* Test valid days for each month type */
    ASSERT(sensor_plot_valid_day(2025, 1, 1));    /* First of month */
    ASSERT(sensor_plot_valid_day(2025, 1, 31));   /* Last of 31-day month */
    ASSERT(sensor_plot_valid_day(2025, 4, 30));   /* Last of 30-day month */
    ASSERT(sensor_plot_valid_day(2025, 2, 28));   /* Last of Feb non-leap */
    ASSERT(sensor_plot_valid_day(2024, 2, 29));   /* Last of Feb leap */
    return true;
}

bool test_invalid_day() {
    ASSERT(!sensor_plot_valid_day(2025, 1, 0));   /* Day 0 */
    ASSERT(!sensor_plot_valid_day(2025, 1, 32));  /* Day 32 in 31-day month */
    ASSERT(!sensor_plot_valid_day(2025, 4, 31));  /* Day 31 in 30-day month */
    ASSERT(!sensor_plot_valid_day(2025, 2, 29));  /* Day 29 in non-leap Feb */
    ASSERT(!sensor_plot_valid_day(2024, 2, 30));  /* Day 30 in leap Feb */
    return true;
}

bool test_valid_hour() {
    for (int h = 0; h <= 23; h++) {
        ASSERT(sensor_plot_valid_hour(h));
    }
    return true;
}

bool test_invalid_hour() {
    ASSERT(!sensor_plot_valid_hour(-1));
    ASSERT(!sensor_plot_valid_hour(24));
    ASSERT(!sensor_plot_valid_hour(25));
    return true;
}

/* ===== Main ===== */

int main() {
    printf("sensor_plot_utils unit tests\n");
    printf("============================\n\n");
    
    printf("Window Duration Tests:\n");
    TEST(window_duration_hour);
    TEST(window_duration_day);
    TEST(window_duration_week);
    TEST(window_duration_month);
    TEST(window_duration_year);
    
    printf("\nStep Size Tests:\n");
    TEST(step_size_hour);
    TEST(step_size_day);
    TEST(step_size_week);
    TEST(step_size_month);
    TEST(step_size_year);
    
    printf("\nLeap Year Tests:\n");
    TEST(leap_year_regular);
    TEST(leap_year_century_not_leap);
    TEST(leap_year_century_leap);
    TEST(not_leap_year);
    
    printf("\nDays in Month Tests:\n");
    TEST(days_in_january);
    TEST(days_in_february_regular);
    TEST(days_in_february_leap);
    TEST(days_in_february_century_not_leap);
    TEST(days_in_30_day_months);
    TEST(days_in_31_day_months);
    TEST(days_in_invalid_month);
    
    printf("\nValidation Tests:\n");
    TEST(valid_year);
    TEST(invalid_year);
    TEST(valid_month);
    TEST(invalid_month);
    TEST(valid_day);
    TEST(invalid_day);
    TEST(valid_hour);
    TEST(invalid_hour);
    
    printf("\n============================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_run);
    
    return (tests_passed == tests_run) ? 0 : 1;
}
