#include "../include/date_utils.h"
#include <cassert>
#include <iostream>
#include <ctime>

void test_parse_unix_timestamp() {
    // Unix timestamp
    long long result = DateUtils::parseDate("1700000000");
    assert(result == 1700000000);
    std::cout << "[PASS] test_parse_unix_timestamp" << std::endl;
}

void test_parse_iso_date() {
    // ISO 8601 date only
    long long result = DateUtils::parseDate("2026-01-17");
    
    // Verify by converting back
    time_t t = static_cast<time_t>(result);
    struct tm* tm = localtime(&t);
    assert(tm->tm_year == 2026 - 1900);
    assert(tm->tm_mon == 0);  // January = 0
    assert(tm->tm_mday == 17);
    std::cout << "[PASS] test_parse_iso_date" << std::endl;
}

void test_parse_iso_datetime() {
    // ISO 8601 with time
    long long result = DateUtils::parseDate("2026-01-17T14:30:00");
    
    time_t t = static_cast<time_t>(result);
    struct tm* tm = localtime(&t);
    assert(tm->tm_year == 2026 - 1900);
    assert(tm->tm_mon == 0);
    assert(tm->tm_mday == 17);
    assert(tm->tm_hour == 14);
    assert(tm->tm_min == 30);
    assert(tm->tm_sec == 0);
    std::cout << "[PASS] test_parse_iso_datetime" << std::endl;
}

void test_parse_uk_date() {
    // DD/MM/YYYY format
    long long result = DateUtils::parseDate("17/01/2026");
    
    time_t t = static_cast<time_t>(result);
    struct tm* tm = localtime(&t);
    assert(tm->tm_year == 2026 - 1900);
    assert(tm->tm_mon == 0);  // January = 0
    assert(tm->tm_mday == 17);
    std::cout << "[PASS] test_parse_uk_date" << std::endl;
}

void test_parse_empty_string() {
    long long result = DateUtils::parseDate("");
    assert(result == 0);
    std::cout << "[PASS] test_parse_empty_string" << std::endl;
}

void test_get_timestamp_from_reading() {
    Reading reading;
    reading["timestamp"] = "2026-01-17T10:00:00";
    reading["sensor"] = "ds18b20";
    reading["value"] = "22.5";
    
    long long ts = DateUtils::getTimestamp(reading);
    assert(ts > 0);
    
    time_t t = static_cast<time_t>(ts);
    struct tm* tm = localtime(&t);
    assert(tm->tm_year == 2026 - 1900);
    assert(tm->tm_hour == 10);
    std::cout << "[PASS] test_get_timestamp_from_reading" << std::endl;
}

void test_get_timestamp_missing() {
    Reading reading;
    reading["sensor"] = "ds18b20";
    reading["value"] = "22.5";
    
    long long ts = DateUtils::getTimestamp(reading);
    assert(ts == 0);
    std::cout << "[PASS] test_get_timestamp_missing" << std::endl;
}

void test_is_in_date_range_within() {
    long long timestamp = DateUtils::parseDate("2026-01-15");
    long long minDate = DateUtils::parseDate("2026-01-10");
    long long maxDate = DateUtils::parseDate("2026-01-20");
    
    assert(DateUtils::isInDateRange(timestamp, minDate, maxDate) == true);
    std::cout << "[PASS] test_is_in_date_range_within" << std::endl;
}

void test_is_in_date_range_before_min() {
    long long timestamp = DateUtils::parseDate("2026-01-05");
    long long minDate = DateUtils::parseDate("2026-01-10");
    long long maxDate = DateUtils::parseDate("2026-01-20");
    
    assert(DateUtils::isInDateRange(timestamp, minDate, maxDate) == false);
    std::cout << "[PASS] test_is_in_date_range_before_min" << std::endl;
}

void test_is_in_date_range_after_max() {
    long long timestamp = DateUtils::parseDate("2026-01-25");
    long long minDate = DateUtils::parseDate("2026-01-10");
    long long maxDate = DateUtils::parseDate("2026-01-20");
    
    assert(DateUtils::isInDateRange(timestamp, minDate, maxDate) == false);
    std::cout << "[PASS] test_is_in_date_range_after_max" << std::endl;
}

void test_is_in_date_range_no_min() {
    long long timestamp = DateUtils::parseDate("2026-01-05");
    long long minDate = 0;  // No min
    long long maxDate = DateUtils::parseDate("2026-01-20");
    
    assert(DateUtils::isInDateRange(timestamp, minDate, maxDate) == true);
    std::cout << "[PASS] test_is_in_date_range_no_min" << std::endl;
}

void test_is_in_date_range_no_max() {
    long long timestamp = DateUtils::parseDate("2026-01-25");
    long long minDate = DateUtils::parseDate("2026-01-10");
    long long maxDate = 0;  // No max
    
    assert(DateUtils::isInDateRange(timestamp, minDate, maxDate) == true);
    std::cout << "[PASS] test_is_in_date_range_no_max" << std::endl;
}

void test_is_in_date_range_no_timestamp() {
    long long timestamp = 0;  // No timestamp in reading
    long long minDate = DateUtils::parseDate("2026-01-10");
    long long maxDate = DateUtils::parseDate("2026-01-20");
    
    // Should include by default when no timestamp
    assert(DateUtils::isInDateRange(timestamp, minDate, maxDate) == true);
    std::cout << "[PASS] test_is_in_date_range_no_timestamp" << std::endl;
}

void test_parse_negative_unix_timestamp() {
    // Negative timestamp (before 1970)
    long long result = DateUtils::parseDate("-86400");
    assert(result == -86400);
    std::cout << "[PASS] test_parse_negative_unix_timestamp" << std::endl;
}

void test_parse_end_of_day_iso_date() {
    // Date without time should use 23:59:59
    long long result = DateUtils::parseDateEndOfDay("2026-01-15");
    long long expected = DateUtils::parseDate("2026-01-15T23:59:59");
    assert(result == expected);
    std::cout << "[PASS] test_parse_end_of_day_iso_date" << std::endl;
}

void test_parse_end_of_day_with_time() {
    // Date with time should use specified time, not end of day
    long long result = DateUtils::parseDateEndOfDay("2026-01-15T10:30:00");
    long long expected = DateUtils::parseDate("2026-01-15T10:30:00");
    assert(result == expected);
    std::cout << "[PASS] test_parse_end_of_day_with_time" << std::endl;
}

void test_parse_end_of_day_uk_date() {
    // UK format without time should use 23:59:59
    long long result = DateUtils::parseDateEndOfDay("15/01/2026");
    long long expected = DateUtils::parseDate("2026-01-15T23:59:59");
    assert(result == expected);
    std::cout << "[PASS] test_parse_end_of_day_uk_date" << std::endl;
}

void test_parse_end_of_day_unix_timestamp() {
    // Unix timestamp should be used as-is
    long long result = DateUtils::parseDateEndOfDay("1737072000");
    assert(result == 1737072000);
    std::cout << "[PASS] test_parse_end_of_day_unix_timestamp" << std::endl;
}

// Date validation tests
void test_validate_valid_datetime() {
    assert(DateUtils::isValidDateTime(2026, 1, 15, 10, 30, 0) == true);
    assert(DateUtils::isValidDateTime(2024, 2, 29, 0, 0, 0) == true);  // Leap year
    assert(DateUtils::isValidDateTime(2000, 2, 29, 0, 0, 0) == true);  // Leap year (divisible by 400)
    assert(DateUtils::isValidDateTime(1970, 1, 1, 0, 0, 0) == true);   // Unix epoch
    std::cout << "[PASS] test_validate_valid_datetime" << std::endl;
}

void test_validate_invalid_year() {
    assert(DateUtils::isValidDateTime(1969, 1, 15, 0, 0, 0) == false);  // Before 1970
    assert(DateUtils::isValidDateTime(2101, 1, 15, 0, 0, 0) == false);  // After 2100
    std::cout << "[PASS] test_validate_invalid_year" << std::endl;
}

void test_validate_invalid_month() {
    assert(DateUtils::isValidDateTime(2026, 0, 15, 0, 0, 0) == false);  // Month 0
    assert(DateUtils::isValidDateTime(2026, 13, 15, 0, 0, 0) == false); // Month 13
    std::cout << "[PASS] test_validate_invalid_month" << std::endl;
}

void test_validate_invalid_day() {
    assert(DateUtils::isValidDateTime(2026, 1, 0, 0, 0, 0) == false);   // Day 0
    assert(DateUtils::isValidDateTime(2026, 1, 32, 0, 0, 0) == false);  // Day 32 in January
    assert(DateUtils::isValidDateTime(2026, 2, 30, 0, 0, 0) == false);  // Feb 30
    assert(DateUtils::isValidDateTime(2026, 2, 29, 0, 0, 0) == false);  // Feb 29 in non-leap year
    assert(DateUtils::isValidDateTime(2026, 4, 31, 0, 0, 0) == false);  // April 31
    assert(DateUtils::isValidDateTime(1900, 2, 29, 0, 0, 0) == false);  // Feb 29 in century year (not divisible by 400)
    std::cout << "[PASS] test_validate_invalid_day" << std::endl;
}

void test_validate_invalid_time() {
    assert(DateUtils::isValidDateTime(2026, 1, 15, 24, 0, 0) == false);  // Hour 24
    assert(DateUtils::isValidDateTime(2026, 1, 15, -1, 0, 0) == false);  // Hour -1
    assert(DateUtils::isValidDateTime(2026, 1, 15, 10, 60, 0) == false); // Minute 60
    assert(DateUtils::isValidDateTime(2026, 1, 15, 10, 30, 60) == false); // Second 60
    std::cout << "[PASS] test_validate_invalid_time" << std::endl;
}

void test_parse_invalid_date_returns_zero() {
    // Invalid formats that should return 0
    assert(DateUtils::parseDate("not-a-date") == 0);
    assert(DateUtils::parseDate("invalid") == 0);
    assert(DateUtils::parseDate("abc123") == 0);
    assert(DateUtils::parseDate("Jan-15-2026") == 0);
    assert(DateUtils::parseDate("yesterday") == 0);
    std::cout << "[PASS] test_parse_invalid_date_returns_zero" << std::endl;
}

void test_parse_invalid_day_value() {
    // Day 32 is invalid
    assert(DateUtils::parseDate("2026-01-32") == 0);
    // Day 0 is invalid
    assert(DateUtils::parseDate("2026-01-00") == 0);
    // Feb 30 is invalid
    assert(DateUtils::parseDate("2026-02-30") == 0);
    // Feb 29 in non-leap year is invalid
    assert(DateUtils::parseDate("2026-02-29") == 0);
    std::cout << "[PASS] test_parse_invalid_day_value" << std::endl;
}

void test_parse_invalid_month_value() {
    // Month 0 is invalid
    assert(DateUtils::parseDate("2026-00-15") == 0);
    // Month 13 is invalid
    assert(DateUtils::parseDate("2026-13-15") == 0);
    std::cout << "[PASS] test_parse_invalid_month_value" << std::endl;
}

void test_parse_invalid_uk_format() {
    // Day 32 in UK format
    assert(DateUtils::parseDate("32/01/2026") == 0);
    // Month 13 in UK format
    assert(DateUtils::parseDate("15/13/2026") == 0);
    // Invalid year-like day (2024 as day)
    assert(DateUtils::parseDate("2024/01/01") == 0);
    std::cout << "[PASS] test_parse_invalid_uk_format" << std::endl;
}

void test_parse_invalid_time_values() {
    // Hour 25 is invalid
    assert(DateUtils::parseDate("2026-01-15T25:00:00") == 0);
    // Minute 60 is invalid
    assert(DateUtils::parseDate("2026-01-15T10:60:00") == 0);
    // Second 60 is invalid
    assert(DateUtils::parseDate("2026-01-15T10:30:60") == 0);
    std::cout << "[PASS] test_parse_invalid_time_values" << std::endl;
}

int main() {
    std::cout << "Running Date Utils Tests..." << std::endl;
    
    // Parsing tests
    test_parse_unix_timestamp();
    test_parse_iso_date();
    test_parse_iso_datetime();
    test_parse_uk_date();
    test_parse_empty_string();
    test_parse_negative_unix_timestamp();
    
    // Reading extraction tests
    test_get_timestamp_from_reading();
    test_get_timestamp_missing();
    
    // Range filter tests
    test_is_in_date_range_within();
    test_is_in_date_range_before_min();
    test_is_in_date_range_after_max();
    test_is_in_date_range_no_min();
    test_is_in_date_range_no_max();
    test_is_in_date_range_no_timestamp();
    
    // End of day parsing tests (for --max-date)
    test_parse_end_of_day_iso_date();
    test_parse_end_of_day_with_time();
    test_parse_end_of_day_uk_date();
    test_parse_end_of_day_unix_timestamp();
    
    // Date/time validation tests
    test_validate_valid_datetime();
    test_validate_invalid_year();
    test_validate_invalid_month();
    test_validate_invalid_day();
    test_validate_invalid_time();
    
    // Invalid date parsing tests
    test_parse_invalid_date_returns_zero();
    test_parse_invalid_day_value();
    test_parse_invalid_month_value();
    test_parse_invalid_uk_format();
    test_parse_invalid_time_values();
    
    std::cout << "All Date Utils tests passed!" << std::endl;
    return 0;
}
