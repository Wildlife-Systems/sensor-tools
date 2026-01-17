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
    std::map<std::string, std::string> reading;
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
    std::map<std::string, std::string> reading;
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
    
    std::cout << "All Date Utils tests passed!" << std::endl;
    return 0;
}
