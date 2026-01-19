/**
 * Unit tests for CommandBase filtering functionality
 */

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <cmath>
#include "../include/command_base.h"

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (!(condition)) { \
            std::cerr << "[FAIL] " << __func__ << ": " << message << std::endl; \
            return false; \
        } \
        tests_passed++; \
        return true; \
    } while(0)

#define ASSERT_TRUE(condition) ASSERT(condition, #condition " should be true")
#define ASSERT_FALSE(condition) ASSERT(!(condition), #condition " should be false")
#define ASSERT_EQ(a, b) ASSERT((a) == (b), #a " should equal " #b)

/**
 * Concrete implementation of CommandBase for testing
 */
class TestableCommand : public CommandBase {
public:
    TestableCommand() : CommandBase() {}
    
    // Expose protected methods for testing
    using CommandBase::shouldIncludeReading;
    using CommandBase::passesDateFilter;
    using CommandBase::areAllReadingsEmpty;
    
    // Expose protected members for testing
    using CommandBase::minDate;
    using CommandBase::maxDate;
    using CommandBase::notEmptyColumns;
    using CommandBase::onlyValueFilters;
    using CommandBase::excludeValueFilters;
    using CommandBase::allowedValues;
    using CommandBase::removeErrors;
    using CommandBase::verbosity;
};

// ==================== passesDateFilter tests ====================

bool test_no_date_filter() {
    TestableCommand cmd;
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"timestamp", "1609459200"}
    };
    ASSERT_TRUE(cmd.passesDateFilter(reading));
}

bool test_min_date_filter_pass() {
    TestableCommand cmd;
    cmd.minDate = 1609459200; // 2021-01-01
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"timestamp", "1609459300"} // After minDate
    };
    ASSERT_TRUE(cmd.passesDateFilter(reading));
}

bool test_min_date_filter_fail() {
    TestableCommand cmd;
    cmd.minDate = 1609459200; // 2021-01-01
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"timestamp", "1609459100"} // Before minDate
    };
    ASSERT_FALSE(cmd.passesDateFilter(reading));
}

bool test_max_date_filter_pass() {
    TestableCommand cmd;
    cmd.maxDate = 1609459200; // 2021-01-01
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"timestamp", "1609459100"} // Before maxDate
    };
    ASSERT_TRUE(cmd.passesDateFilter(reading));
}

bool test_max_date_filter_fail() {
    TestableCommand cmd;
    cmd.maxDate = 1609459200; // 2021-01-01
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"timestamp", "1609459300"} // After maxDate
    };
    ASSERT_FALSE(cmd.passesDateFilter(reading));
}

bool test_date_range_filter_pass() {
    TestableCommand cmd;
    cmd.minDate = 1609459100;
    cmd.maxDate = 1609459300;
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"timestamp", "1609459200"} // Within range
    };
    ASSERT_TRUE(cmd.passesDateFilter(reading));
}

bool test_date_range_filter_fail() {
    TestableCommand cmd;
    cmd.minDate = 1609459100;
    cmd.maxDate = 1609459200;
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"timestamp", "1609459300"} // Outside range
    };
    ASSERT_FALSE(cmd.passesDateFilter(reading));
}

// ==================== areAllReadingsEmpty tests ====================

bool test_all_readings_empty_true() {
    std::vector<std::map<std::string, std::string>> readings = {
        {},
        {},
        {}
    };
    ASSERT_TRUE(TestableCommand::areAllReadingsEmpty(readings));
}

bool test_all_readings_empty_false() {
    std::vector<std::map<std::string, std::string>> readings = {
        {},
        {{"key", "value"}},
        {}
    };
    ASSERT_FALSE(TestableCommand::areAllReadingsEmpty(readings));
}

bool test_all_readings_empty_empty_vector() {
    std::vector<std::map<std::string, std::string>> readings;
    ASSERT_TRUE(TestableCommand::areAllReadingsEmpty(readings));
}

// ==================== shouldIncludeReading tests ====================

bool test_include_basic_reading() {
    TestableCommand cmd;
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"value", "25.5"}
    };
    ASSERT_TRUE(cmd.shouldIncludeReading(reading));
}

bool test_exclude_by_date() {
    TestableCommand cmd;
    cmd.minDate = 1609459200;
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"timestamp", "1609459100"}
    };
    ASSERT_FALSE(cmd.shouldIncludeReading(reading));
}

bool test_not_empty_column_present() {
    TestableCommand cmd;
    cmd.notEmptyColumns.insert("value");
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"value", "25.5"}
    };
    ASSERT_TRUE(cmd.shouldIncludeReading(reading));
}

bool test_not_empty_column_missing() {
    TestableCommand cmd;
    cmd.notEmptyColumns.insert("value");
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"}
    };
    ASSERT_FALSE(cmd.shouldIncludeReading(reading));
}

bool test_not_empty_column_empty_value() {
    TestableCommand cmd;
    cmd.notEmptyColumns.insert("value");
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"value", ""}
    };
    ASSERT_FALSE(cmd.shouldIncludeReading(reading));
}

bool test_only_value_filter_pass() {
    TestableCommand cmd;
    cmd.onlyValueFilters["status"].insert("active");
    cmd.onlyValueFilters["status"].insert("pending");
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"status", "active"}
    };
    ASSERT_TRUE(cmd.shouldIncludeReading(reading));
}

bool test_only_value_filter_fail() {
    TestableCommand cmd;
    cmd.onlyValueFilters["status"].insert("active");
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"status", "inactive"}
    };
    ASSERT_FALSE(cmd.shouldIncludeReading(reading));
}

bool test_only_value_filter_missing_column() {
    TestableCommand cmd;
    cmd.onlyValueFilters["status"].insert("active");
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"}
    };
    ASSERT_FALSE(cmd.shouldIncludeReading(reading));
}

bool test_exclude_value_filter_pass() {
    TestableCommand cmd;
    cmd.excludeValueFilters["status"].insert("error");
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"status", "active"}
    };
    ASSERT_TRUE(cmd.shouldIncludeReading(reading));
}

bool test_exclude_value_filter_fail() {
    TestableCommand cmd;
    cmd.excludeValueFilters["status"].insert("error");
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"status", "error"}
    };
    ASSERT_FALSE(cmd.shouldIncludeReading(reading));
}

bool test_exclude_value_filter_missing_column() {
    TestableCommand cmd;
    cmd.excludeValueFilters["status"].insert("error");
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"}
    };
    // Missing column should pass exclude filter
    ASSERT_TRUE(cmd.shouldIncludeReading(reading));
}

bool test_allowed_values_filter_pass() {
    TestableCommand cmd;
    cmd.allowedValues["type"].insert("temperature");
    cmd.allowedValues["type"].insert("humidity");
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"type", "temperature"}
    };
    ASSERT_TRUE(cmd.shouldIncludeReading(reading));
}

bool test_allowed_values_filter_fail() {
    TestableCommand cmd;
    cmd.allowedValues["type"].insert("temperature");
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"type", "pressure"}
    };
    ASSERT_FALSE(cmd.shouldIncludeReading(reading));
}

bool test_remove_errors_disabled() {
    TestableCommand cmd;
    cmd.removeErrors = false;
    std::map<std::string, std::string> reading = {
        {"sensor", "ds18b20"},
        {"sensor_id", "28-000000000000"},
        {"value", "85"}
    };
    ASSERT_TRUE(cmd.shouldIncludeReading(reading));
}

bool test_remove_errors_enabled_with_error() {
    TestableCommand cmd;
    cmd.removeErrors = true;
    std::map<std::string, std::string> reading = {
        {"sensor", "ds18b20"},
        {"sensor_id", "28-000000000000"},
        {"value", "85"}
    };
    ASSERT_FALSE(cmd.shouldIncludeReading(reading));
}

bool test_remove_errors_enabled_no_error() {
    TestableCommand cmd;
    cmd.removeErrors = true;
    std::map<std::string, std::string> reading = {
        {"sensor", "ds18b20"},
        {"sensor_id", "28-000000000000"},
        {"value", "25.500"}
    };
    ASSERT_TRUE(cmd.shouldIncludeReading(reading));
}

bool test_multiple_filters_all_pass() {
    TestableCommand cmd;
    cmd.notEmptyColumns.insert("value");
    cmd.onlyValueFilters["type"].insert("temperature");
    cmd.excludeValueFilters["status"].insert("error");
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"value", "25.5"},
        {"type", "temperature"},
        {"status", "active"}
    };
    ASSERT_TRUE(cmd.shouldIncludeReading(reading));
}

bool test_multiple_filters_one_fails() {
    TestableCommand cmd;
    cmd.notEmptyColumns.insert("value");
    cmd.onlyValueFilters["type"].insert("temperature");
    cmd.excludeValueFilters["status"].insert("error");
    std::map<std::string, std::string> reading = {
        {"sensor_id", "sensor1"},
        {"value", "25.5"},
        {"type", "temperature"},
        {"status", "error"} // This should cause exclusion
    };
    ASSERT_FALSE(cmd.shouldIncludeReading(reading));
}

// ==================== Main ====================

int main() {
    std::cout << "Running CommandBase unit tests..." << std::endl;
    
    // Date filter tests
    std::cout << (test_no_date_filter() ? "[PASS]" : "[FAIL]") << " test_no_date_filter" << std::endl;
    std::cout << (test_min_date_filter_pass() ? "[PASS]" : "[FAIL]") << " test_min_date_filter_pass" << std::endl;
    std::cout << (test_min_date_filter_fail() ? "[PASS]" : "[FAIL]") << " test_min_date_filter_fail" << std::endl;
    std::cout << (test_max_date_filter_pass() ? "[PASS]" : "[FAIL]") << " test_max_date_filter_pass" << std::endl;
    std::cout << (test_max_date_filter_fail() ? "[PASS]" : "[FAIL]") << " test_max_date_filter_fail" << std::endl;
    std::cout << (test_date_range_filter_pass() ? "[PASS]" : "[FAIL]") << " test_date_range_filter_pass" << std::endl;
    std::cout << (test_date_range_filter_fail() ? "[PASS]" : "[FAIL]") << " test_date_range_filter_fail" << std::endl;
    
    // areAllReadingsEmpty tests
    std::cout << (test_all_readings_empty_true() ? "[PASS]" : "[FAIL]") << " test_all_readings_empty_true" << std::endl;
    std::cout << (test_all_readings_empty_false() ? "[PASS]" : "[FAIL]") << " test_all_readings_empty_false" << std::endl;
    std::cout << (test_all_readings_empty_empty_vector() ? "[PASS]" : "[FAIL]") << " test_all_readings_empty_empty_vector" << std::endl;
    
    // shouldIncludeReading tests
    std::cout << (test_include_basic_reading() ? "[PASS]" : "[FAIL]") << " test_include_basic_reading" << std::endl;
    std::cout << (test_exclude_by_date() ? "[PASS]" : "[FAIL]") << " test_exclude_by_date" << std::endl;
    std::cout << (test_not_empty_column_present() ? "[PASS]" : "[FAIL]") << " test_not_empty_column_present" << std::endl;
    std::cout << (test_not_empty_column_missing() ? "[PASS]" : "[FAIL]") << " test_not_empty_column_missing" << std::endl;
    std::cout << (test_not_empty_column_empty_value() ? "[PASS]" : "[FAIL]") << " test_not_empty_column_empty_value" << std::endl;
    std::cout << (test_only_value_filter_pass() ? "[PASS]" : "[FAIL]") << " test_only_value_filter_pass" << std::endl;
    std::cout << (test_only_value_filter_fail() ? "[PASS]" : "[FAIL]") << " test_only_value_filter_fail" << std::endl;
    std::cout << (test_only_value_filter_missing_column() ? "[PASS]" : "[FAIL]") << " test_only_value_filter_missing_column" << std::endl;
    std::cout << (test_exclude_value_filter_pass() ? "[PASS]" : "[FAIL]") << " test_exclude_value_filter_pass" << std::endl;
    std::cout << (test_exclude_value_filter_fail() ? "[PASS]" : "[FAIL]") << " test_exclude_value_filter_fail" << std::endl;
    std::cout << (test_exclude_value_filter_missing_column() ? "[PASS]" : "[FAIL]") << " test_exclude_value_filter_missing_column" << std::endl;
    std::cout << (test_allowed_values_filter_pass() ? "[PASS]" : "[FAIL]") << " test_allowed_values_filter_pass" << std::endl;
    std::cout << (test_allowed_values_filter_fail() ? "[PASS]" : "[FAIL]") << " test_allowed_values_filter_fail" << std::endl;
    std::cout << (test_remove_errors_disabled() ? "[PASS]" : "[FAIL]") << " test_remove_errors_disabled" << std::endl;
    std::cout << (test_remove_errors_enabled_with_error() ? "[PASS]" : "[FAIL]") << " test_remove_errors_enabled_with_error" << std::endl;
    std::cout << (test_remove_errors_enabled_no_error() ? "[PASS]" : "[FAIL]") << " test_remove_errors_enabled_no_error" << std::endl;
    std::cout << (test_multiple_filters_all_pass() ? "[PASS]" : "[FAIL]") << " test_multiple_filters_all_pass" << std::endl;
    std::cout << (test_multiple_filters_one_fails() ? "[PASS]" : "[FAIL]") << " test_multiple_filters_one_fails" << std::endl;
    
    std::cout << "\n" << tests_passed << "/" << tests_run << " tests passed" << std::endl;
    
    return tests_passed == tests_run ? 0 : 1;
}
