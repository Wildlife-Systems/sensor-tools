/* sensor_plot_utils.h - Utility functions for sensor-plot
 * 
 * These functions are separated out to allow unit testing.
 */

#ifndef SENSOR_PLOT_UTILS_H
#define SENSOR_PLOT_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/* View mode definitions */
typedef enum {
    SENSOR_PLOT_MODE_HOUR,
    SENSOR_PLOT_MODE_DAY,
    SENSOR_PLOT_MODE_WEEK,
    SENSOR_PLOT_MODE_MONTH,
    SENSOR_PLOT_MODE_YEAR
} sensor_plot_mode_t;

/* Get window duration in seconds based on mode */
static inline long sensor_plot_get_window_duration(sensor_plot_mode_t mode)
{
    switch (mode) {
        case SENSOR_PLOT_MODE_HOUR:  return 3600L;              /* 1 hour */
        case SENSOR_PLOT_MODE_DAY:   return 24L * 3600L;        /* 24 hours */
        case SENSOR_PLOT_MODE_WEEK:  return 7L * 24L * 3600L;   /* 7 days */
        case SENSOR_PLOT_MODE_MONTH: return 30L * 24L * 3600L;  /* 30 days */
        case SENSOR_PLOT_MODE_YEAR:  return 365L * 24L * 3600L; /* 365 days */
        default:                     return 24L * 3600L;
    }
}

/* Get step size in seconds based on mode */
static inline long sensor_plot_get_step_size(sensor_plot_mode_t mode)
{
    switch (mode) {
        case SENSOR_PLOT_MODE_HOUR:  return 60L;              /* 1 minute */
        case SENSOR_PLOT_MODE_DAY:   return 3600L;            /* 1 hour */
        case SENSOR_PLOT_MODE_WEEK:  return 24L * 3600L;      /* 1 day */
        case SENSOR_PLOT_MODE_MONTH: return 7L * 24L * 3600L; /* 1 week */
        case SENSOR_PLOT_MODE_YEAR:  return 30L * 24L * 3600L;/* 1 month */
        default:                     return 3600L;
    }
}

/* Check if a year is a leap year */
static inline int sensor_plot_is_leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/* Get number of days in a given month (1-12) for a given year */
static inline int sensor_plot_days_in_month(int year, int month)
{
    if (month < 1 || month > 12) return 0;
    
    if (month == 2) {
        return sensor_plot_is_leap_year(year) ? 29 : 28;
    } else if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    } else {
        return 31;
    }
}

/* Validate year range */
static inline int sensor_plot_valid_year(int year)
{
    return year >= 1970 && year <= 2100;
}

/* Validate month range */
static inline int sensor_plot_valid_month(int month)
{
    return month >= 1 && month <= 12;
}

/* Validate day for a given year and month */
static inline int sensor_plot_valid_day(int year, int month, int day)
{
    if (day < 1) return 0;
    return day <= sensor_plot_days_in_month(year, month);
}

/* Validate hour range */
static inline int sensor_plot_valid_hour(int hour)
{
    return hour >= 0 && hour <= 23;
}

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_PLOT_UTILS_H */
