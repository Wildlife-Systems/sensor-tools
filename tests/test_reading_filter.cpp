#include "../include/reading_filter.h"
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

// ===== Date Filter Tests =====

void test_date_filter_no_filter() {
    ReadingFilter filter;
    Reading reading = {{"sensor_id", "s1"}, {"timestamp", "1000"}, {"value", "22.5"}};
    
    assert(filter.shouldInclude(reading) == true);
    std::cout << "[PASS] test_date_filter_no_filter" << std::endl;
}

void test_date_filter_min_only() {
    ReadingFilter filter;
    filter.setDateRange(500, 0);  // min only
    
    Reading reading1 = {{"sensor_id", "s1"}, {"timestamp", "1000"}, {"value", "22.5"}};  // passes
    Reading reading2 = {{"sensor_id", "s2"}, {"timestamp", "200"}, {"value", "23.0"}};   // fails
    
    assert(filter.shouldInclude(reading1) == true);
    assert(filter.shouldInclude(reading2) == false);
    std::cout << "[PASS] test_date_filter_min_only" << std::endl;
}

void test_date_filter_max_only() {
    ReadingFilter filter;
    filter.setDateRange(0, 500);  // max only
    
    Reading reading1 = {{"sensor_id", "s1"}, {"timestamp", "200"}, {"value", "22.5"}};   // passes
    Reading reading2 = {{"sensor_id", "s2"}, {"timestamp", "1000"}, {"value", "23.0"}};  // fails
    
    assert(filter.shouldInclude(reading1) == true);
    assert(filter.shouldInclude(reading2) == false);
    std::cout << "[PASS] test_date_filter_max_only" << std::endl;
}

void test_date_filter_range() {
    ReadingFilter filter;
    filter.setDateRange(200, 800);  // range
    
    Reading reading1 = {{"sensor_id", "s1"}, {"timestamp", "100"}, {"value", "22.5"}};   // before
    Reading reading2 = {{"sensor_id", "s2"}, {"timestamp", "500"}, {"value", "23.0"}};   // inside
    Reading reading3 = {{"sensor_id", "s3"}, {"timestamp", "1000"}, {"value", "24.0"}};  // after
    
    assert(filter.shouldInclude(reading1) == false);
    assert(filter.shouldInclude(reading2) == true);
    assert(filter.shouldInclude(reading3) == false);
    std::cout << "[PASS] test_date_filter_range" << std::endl;
}

void test_date_filter_boundary() {
    ReadingFilter filter;
    filter.setDateRange(200, 800);
    
    Reading reading1 = {{"sensor_id", "s1"}, {"timestamp", "200"}, {"value", "22.5"}};  // exactly min
    Reading reading2 = {{"sensor_id", "s2"}, {"timestamp", "800"}, {"value", "23.0"}};  // exactly max
    
    assert(filter.shouldInclude(reading1) == true);
    assert(filter.shouldInclude(reading2) == true);
    std::cout << "[PASS] test_date_filter_boundary" << std::endl;
}

// ===== Not Empty Filter Tests =====

void test_not_empty_filter_passes() {
    ReadingFilter filter;
    filter.addNotEmptyColumn("value");
    
    Reading reading = {{"sensor_id", "s1"}, {"value", "22.5"}};
    assert(filter.shouldInclude(reading) == true);
    std::cout << "[PASS] test_not_empty_filter_passes" << std::endl;
}

void test_not_empty_filter_fails_empty() {
    ReadingFilter filter;
    filter.addNotEmptyColumn("value");
    
    Reading reading = {{"sensor_id", "s1"}, {"value", ""}};
    assert(filter.shouldInclude(reading) == false);
    std::cout << "[PASS] test_not_empty_filter_fails_empty" << std::endl;
}

void test_not_empty_filter_fails_missing() {
    ReadingFilter filter;
    filter.addNotEmptyColumn("value");
    
    Reading reading = {{"sensor_id", "s1"}};  // no value column
    assert(filter.shouldInclude(reading) == false);
    std::cout << "[PASS] test_not_empty_filter_fails_missing" << std::endl;
}

void test_not_empty_filter_multiple_columns() {
    ReadingFilter filter;
    filter.addNotEmptyColumn("value");
    filter.addNotEmptyColumn("sensor_id");
    
    Reading reading1 = {{"sensor_id", "s1"}, {"value", "22.5"}};  // both present
    Reading reading2 = {{"sensor_id", ""}, {"value", "22.5"}};    // sensor_id empty
    Reading reading3 = {{"sensor_id", "s1"}, {"value", ""}};      // value empty
    
    assert(filter.shouldInclude(reading1) == true);
    assert(filter.shouldInclude(reading2) == false);
    assert(filter.shouldInclude(reading3) == false);
    std::cout << "[PASS] test_not_empty_filter_multiple_columns" << std::endl;
}

// ===== Not Null Filter Tests =====

void test_not_null_filter_passes() {
    ReadingFilter filter;
    filter.addNotNullColumn("value");
    
    Reading reading = {{"sensor_id", "s1"}, {"value", "22.5"}};
    assert(filter.shouldInclude(reading) == true);
    std::cout << "[PASS] test_not_null_filter_passes" << std::endl;
}

void test_not_null_filter_fails_null_string() {
    ReadingFilter filter;
    filter.addNotNullColumn("value");
    
    Reading reading = {{"sensor_id", "s1"}, {"value", "null"}};
    assert(filter.shouldInclude(reading) == false);
    std::cout << "[PASS] test_not_null_filter_fails_null_string" << std::endl;
}

void test_not_null_filter_passes_missing() {
    ReadingFilter filter;
    filter.addNotNullColumn("value");
    
    // Missing column is NOT null - it's just missing
    Reading reading = {{"sensor_id", "s1"}};
    assert(filter.shouldInclude(reading) == true);
    std::cout << "[PASS] test_not_null_filter_passes_missing" << std::endl;
}

// ===== Only Value Filter Tests =====

void test_only_value_filter_single() {
    ReadingFilter filter;
    filter.addOnlyValueFilter("sensor_id", "s1");
    
    Reading reading1 = {{"sensor_id", "s1"}, {"value", "22.5"}};  // matches
    Reading reading2 = {{"sensor_id", "s2"}, {"value", "23.0"}};  // doesn't match
    
    assert(filter.shouldInclude(reading1) == true);
    assert(filter.shouldInclude(reading2) == false);
    std::cout << "[PASS] test_only_value_filter_single" << std::endl;
}

void test_only_value_filter_multiple_values() {
    ReadingFilter filter;
    filter.addOnlyValueFilter("sensor_id", "s1");
    filter.addOnlyValueFilter("sensor_id", "s2");
    
    Reading reading1 = {{"sensor_id", "s1"}, {"value", "22.5"}};
    Reading reading2 = {{"sensor_id", "s2"}, {"value", "23.0"}};
    Reading reading3 = {{"sensor_id", "s3"}, {"value", "24.0"}};
    
    assert(filter.shouldInclude(reading1) == true);
    assert(filter.shouldInclude(reading2) == true);
    assert(filter.shouldInclude(reading3) == false);
    std::cout << "[PASS] test_only_value_filter_multiple_values" << std::endl;
}

void test_only_value_filter_fails_missing_column() {
    ReadingFilter filter;
    filter.addOnlyValueFilter("sensor_id", "s1");
    
    Reading reading = {{"value", "22.5"}};  // no sensor_id
    assert(filter.shouldInclude(reading) == false);
    std::cout << "[PASS] test_only_value_filter_fails_missing_column" << std::endl;
}

// ===== Exclude Value Filter Tests =====

void test_exclude_value_filter_single() {
    ReadingFilter filter;
    filter.addExcludeValueFilter("sensor_id", "s1");
    
    Reading reading1 = {{"sensor_id", "s1"}, {"value", "22.5"}};  // excluded
    Reading reading2 = {{"sensor_id", "s2"}, {"value", "23.0"}};  // passes
    
    assert(filter.shouldInclude(reading1) == false);
    assert(filter.shouldInclude(reading2) == true);
    std::cout << "[PASS] test_exclude_value_filter_single" << std::endl;
}

void test_exclude_value_filter_multiple_values() {
    ReadingFilter filter;
    filter.addExcludeValueFilter("sensor_id", "s1");
    filter.addExcludeValueFilter("sensor_id", "s2");
    
    Reading reading1 = {{"sensor_id", "s1"}, {"value", "22.5"}};  // excluded
    Reading reading2 = {{"sensor_id", "s2"}, {"value", "23.0"}};  // excluded
    Reading reading3 = {{"sensor_id", "s3"}, {"value", "24.0"}};  // passes
    
    assert(filter.shouldInclude(reading1) == false);
    assert(filter.shouldInclude(reading2) == false);
    assert(filter.shouldInclude(reading3) == true);
    std::cout << "[PASS] test_exclude_value_filter_multiple_values" << std::endl;
}

void test_exclude_value_filter_passes_missing_column() {
    ReadingFilter filter;
    filter.addExcludeValueFilter("sensor_id", "s1");
    
    Reading reading = {{"value", "22.5"}};  // no sensor_id
    assert(filter.shouldInclude(reading) == true);
    std::cout << "[PASS] test_exclude_value_filter_passes_missing_column" << std::endl;
}

// ===== Allowed Values Filter Tests =====

void test_allowed_values_filter_single() {
    ReadingFilter filter;
    filter.addAllowedValue("status", "active");
    
    Reading reading1 = {{"sensor_id", "s1"}, {"status", "active"}};    // allowed
    Reading reading2 = {{"sensor_id", "s2"}, {"status", "inactive"}};  // not allowed
    
    assert(filter.shouldInclude(reading1) == true);
    assert(filter.shouldInclude(reading2) == false);
    std::cout << "[PASS] test_allowed_values_filter_single" << std::endl;
}

void test_allowed_values_filter_multiple() {
    ReadingFilter filter;
    filter.addAllowedValue("status", "active");
    filter.addAllowedValue("status", "pending");
    
    Reading reading1 = {{"sensor_id", "s1"}, {"status", "active"}};   // allowed
    Reading reading2 = {{"sensor_id", "s2"}, {"status", "pending"}};  // allowed
    Reading reading3 = {{"sensor_id", "s3"}, {"status", "error"}};    // not allowed
    
    assert(filter.shouldInclude(reading1) == true);
    assert(filter.shouldInclude(reading2) == true);
    assert(filter.shouldInclude(reading3) == false);
    std::cout << "[PASS] test_allowed_values_filter_multiple" << std::endl;
}

// ===== Remove Errors Filter Tests =====

void test_remove_errors_disabled() {
    ReadingFilter filter;
    filter.setRemoveErrors(false);
    
    // Error reading (temperature 85 is a DS18B20 error)
    Reading reading = {{"sensor_id", "s1"}, {"temperature", "85"}};
    assert(filter.shouldInclude(reading) == true);
    std::cout << "[PASS] test_remove_errors_disabled" << std::endl;
}

void test_remove_errors_enabled() {
    ReadingFilter filter;
    filter.setRemoveErrors(true);
    
    // Error reading (temperature 85 is a DS18B20 error - needs "sensor" field with sensor type)
    Reading errorReading = {{"sensor", "ds18b20"}, {"sensor_id", "s1"}, {"temperature", "85"}};
    // Normal reading
    Reading normalReading = {{"sensor", "ds18b20"}, {"sensor_id", "s2"}, {"temperature", "22.5"}};
    
    assert(filter.shouldInclude(errorReading) == false);
    assert(filter.shouldInclude(normalReading) == true);
    std::cout << "[PASS] test_remove_errors_enabled" << std::endl;
}

// ===== Invert Filter Tests =====

void test_invert_filter_mode() {
    ReadingFilter filter;
    filter.addOnlyValueFilter("sensor_id", "s1");
    filter.setInvertFilter(true);
    
    Reading reading1 = {{"sensor_id", "s1"}, {"value", "22.5"}};  // normally passes, inverted fails
    Reading reading2 = {{"sensor_id", "s2"}, {"value", "23.0"}};  // normally fails, inverted passes
    
    assert(filter.shouldInclude(reading1) == false);
    assert(filter.shouldInclude(reading2) == true);
    std::cout << "[PASS] test_invert_filter_mode" << std::endl;
}

void test_invert_filter_with_date() {
    ReadingFilter filter;
    filter.setDateRange(500, 1000);
    filter.setInvertFilter(true);
    
    Reading insideRange = {{"sensor_id", "s1"}, {"timestamp", "700"}, {"value", "22.5"}};
    Reading outsideRange = {{"sensor_id", "s2"}, {"timestamp", "100"}, {"value", "23.0"}};
    
    assert(filter.shouldInclude(insideRange) == false);   // normally passes, inverted fails
    assert(filter.shouldInclude(outsideRange) == true);   // normally fails, inverted passes
    std::cout << "[PASS] test_invert_filter_with_date" << std::endl;
}

// ===== Unique Rows Filter Tests =====

void test_unique_rows_basic() {
    ReadingFilter filter;
    filter.setUniqueRows(true);
    
    Reading reading1 = {{"sensor_id", "s1"}, {"value", "22.5"}};
    Reading reading2 = {{"sensor_id", "s1"}, {"value", "22.5"}};  // duplicate
    Reading reading3 = {{"sensor_id", "s2"}, {"value", "22.5"}};  // different sensor_id
    
    assert(filter.shouldInclude(reading1) == true);
    assert(filter.shouldInclude(reading2) == false);  // duplicate
    assert(filter.shouldInclude(reading3) == true);   // different
    std::cout << "[PASS] test_unique_rows_basic" << std::endl;
}

void test_unique_rows_order_independent() {
    ReadingFilter filter;
    filter.setUniqueRows(true);
    
    // Same data, different insertion order - should be considered duplicates
    Reading reading1 = {{"a", "1"}, {"b", "2"}, {"c", "3"}};
    Reading reading2 = {{"c", "3"}, {"a", "1"}, {"b", "2"}};  // same data, different order
    
    assert(filter.shouldInclude(reading1) == true);
    assert(filter.shouldInclude(reading2) == false);  // duplicate
    std::cout << "[PASS] test_unique_rows_order_independent" << std::endl;
}

void test_unique_rows_clear() {
    ReadingFilter filter;
    filter.setUniqueRows(true);
    
    Reading reading = {{"sensor_id", "s1"}, {"value", "22.5"}};
    
    assert(filter.shouldInclude(reading) == true);
    assert(filter.shouldInclude(reading) == false);  // duplicate
    
    filter.clearSeenRows();
    
    assert(filter.shouldInclude(reading) == true);   // reset, should pass again
    std::cout << "[PASS] test_unique_rows_clear" << std::endl;
}

// ===== Update Rule Tests =====

void test_update_rule_basic() {
    ReadingFilter filter;
    filter.addUpdateRule(UpdateRule("status", "error", "value", "N/A"));
    
    Reading reading = {{"sensor_id", "s1"}, {"status", "error"}, {"value", "22.5"}};
    filter.applyTransformations(reading);
    
    assert(reading["value"] == "N/A");
    std::cout << "[PASS] test_update_rule_basic" << std::endl;
}

void test_update_rule_no_match() {
    ReadingFilter filter;
    filter.addUpdateRule(UpdateRule("status", "error", "value", "N/A"));
    
    Reading reading = {{"sensor_id", "s1"}, {"status", "ok"}, {"value", "22.5"}};
    filter.applyTransformations(reading);
    
    assert(reading["value"] == "22.5");  // unchanged
    std::cout << "[PASS] test_update_rule_no_match" << std::endl;
}

void test_update_rule_only_when_empty() {
    ReadingFilter filter;
    filter.addUpdateRule(UpdateRule("sensor_type", "temp", "unit", "C", true));  // onlyWhenEmpty
    
    Reading reading1 = {{"sensor_type", "temp"}, {"value", "22.5"}};           // unit missing
    Reading reading2 = {{"sensor_type", "temp"}, {"value", "22.5"}, {"unit", "F"}};  // unit present
    
    filter.applyTransformations(reading1);
    filter.applyTransformations(reading2);
    
    assert(reading1["unit"] == "C");    // set because missing
    assert(reading2["unit"] == "F");    // unchanged because not empty
    std::cout << "[PASS] test_update_rule_only_when_empty" << std::endl;
}

void test_update_rule_only_when_empty_empty_string() {
    ReadingFilter filter;
    filter.addUpdateRule(UpdateRule("sensor_type", "temp", "unit", "C", true));
    
    Reading reading = {{"sensor_type", "temp"}, {"value", "22.5"}, {"unit", ""}};  // unit empty
    filter.applyTransformations(reading);
    
    assert(reading["unit"] == "C");  // set because empty
    std::cout << "[PASS] test_update_rule_only_when_empty_empty_string" << std::endl;
}

void test_update_rule_multiple() {
    ReadingFilter filter;
    filter.addUpdateRule(UpdateRule("status", "error", "value", "N/A"));
    filter.addUpdateRule(UpdateRule("status", "error", "quality", "bad"));
    
    Reading reading = {{"sensor_id", "s1"}, {"status", "error"}, {"value", "22.5"}};
    filter.applyTransformations(reading);
    
    assert(reading["value"] == "N/A");
    assert(reading["quality"] == "bad");
    std::cout << "[PASS] test_update_rule_multiple" << std::endl;
}

// ===== Combined Filter Tests =====

void test_combined_date_and_value_filter() {
    ReadingFilter filter;
    filter.setDateRange(100, 500);
    filter.addOnlyValueFilter("status", "active");
    
    Reading passes = {{"timestamp", "300"}, {"status", "active"}, {"value", "22.5"}};
    Reading fails_date = {{"timestamp", "50"}, {"status", "active"}, {"value", "22.5"}};
    Reading fails_value = {{"timestamp", "300"}, {"status", "error"}, {"value", "22.5"}};
    
    assert(filter.shouldInclude(passes) == true);
    assert(filter.shouldInclude(fails_date) == false);
    assert(filter.shouldInclude(fails_value) == false);
    std::cout << "[PASS] test_combined_date_and_value_filter" << std::endl;
}

void test_combined_include_and_exclude_filter() {
    ReadingFilter filter;
    filter.addOnlyValueFilter("type", "sensor");
    filter.addExcludeValueFilter("status", "error");
    
    Reading passes = {{"type", "sensor"}, {"status", "ok"}, {"value", "22.5"}};
    Reading fails_type = {{"type", "config"}, {"status", "ok"}, {"value", "22.5"}};
    Reading fails_status = {{"type", "sensor"}, {"status", "error"}, {"value", "22.5"}};
    
    assert(filter.shouldInclude(passes) == true);
    assert(filter.shouldInclude(fails_type) == false);
    assert(filter.shouldInclude(fails_status) == false);
    std::cout << "[PASS] test_combined_include_and_exclude_filter" << std::endl;
}

void test_combined_unique_and_value_filter() {
    ReadingFilter filter;
    filter.addOnlyValueFilter("type", "sensor");
    filter.setUniqueRows(true);
    
    Reading reading1 = {{"type", "sensor"}, {"value", "22.5"}};
    Reading reading2 = {{"type", "sensor"}, {"value", "22.5"}};  // duplicate
    Reading reading3 = {{"type", "config"}, {"value", "22.5"}};  // fails type filter
    
    assert(filter.shouldInclude(reading1) == true);
    assert(filter.shouldInclude(reading2) == false);  // duplicate
    assert(filter.shouldInclude(reading3) == false);  // wrong type
    std::cout << "[PASS] test_combined_unique_and_value_filter" << std::endl;
}

// ===== Thread Safety Tests =====

void test_unique_rows_thread_safety() {
    ReadingFilter filter;
    filter.setUniqueRows(true);
    
    std::atomic<int> passCount{0};
    std::atomic<int> failCount{0};
    
    // Run many threads trying to insert the same reading
    std::vector<std::thread> threads;
    for (int t = 0; t < 10; ++t) {
        threads.emplace_back([&filter, &passCount, &failCount]() {
            for (int i = 0; i < 100; ++i) {
                Reading reading = {{"id", std::to_string(i)}, {"value", "test"}};
                if (filter.shouldInclude(reading)) {
                    passCount++;
                } else {
                    failCount++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // With 10 threads each inserting 100 unique readings (ids 0-99),
    // we should see exactly 100 passes (one per unique id) and 900 fails
    assert(passCount == 100);
    assert(failCount == 900);
    std::cout << "[PASS] test_unique_rows_thread_safety" << std::endl;
}

// ===== Bulk Setter Tests =====

void test_bulk_setters() {
    ReadingFilter filter;
    
    std::set<std::string> notEmpty = {"value", "sensor_id"};
    filter.setNotEmptyColumns(notEmpty);
    
    std::map<std::string, std::set<std::string>> onlyValues;
    onlyValues["type"].insert("sensor");
    filter.setOnlyValueFilters(onlyValues);
    
    Reading passes = {{"sensor_id", "s1"}, {"value", "22.5"}, {"type", "sensor"}};
    Reading fails = {{"sensor_id", "s1"}, {"value", ""}, {"type", "sensor"}};
    
    assert(filter.shouldInclude(passes) == true);
    assert(filter.shouldInclude(fails) == false);
    std::cout << "[PASS] test_bulk_setters" << std::endl;
}

int main() {
    std::cout << "Running ReadingFilter Tests..." << std::endl;
    
    // Date filter tests
    test_date_filter_no_filter();
    test_date_filter_min_only();
    test_date_filter_max_only();
    test_date_filter_range();
    test_date_filter_boundary();
    
    // Not empty filter tests
    test_not_empty_filter_passes();
    test_not_empty_filter_fails_empty();
    test_not_empty_filter_fails_missing();
    test_not_empty_filter_multiple_columns();
    
    // Not null filter tests
    test_not_null_filter_passes();
    test_not_null_filter_fails_null_string();
    test_not_null_filter_passes_missing();
    
    // Only value filter tests
    test_only_value_filter_single();
    test_only_value_filter_multiple_values();
    test_only_value_filter_fails_missing_column();
    
    // Exclude value filter tests
    test_exclude_value_filter_single();
    test_exclude_value_filter_multiple_values();
    test_exclude_value_filter_passes_missing_column();
    
    // Allowed values filter tests
    test_allowed_values_filter_single();
    test_allowed_values_filter_multiple();
    
    // Remove errors filter tests
    test_remove_errors_disabled();
    test_remove_errors_enabled();
    
    // Invert filter tests
    test_invert_filter_mode();
    test_invert_filter_with_date();
    
    // Unique rows filter tests
    test_unique_rows_basic();
    test_unique_rows_order_independent();
    test_unique_rows_clear();
    
    // Update rule tests
    test_update_rule_basic();
    test_update_rule_no_match();
    test_update_rule_only_when_empty();
    test_update_rule_only_when_empty_empty_string();
    test_update_rule_multiple();
    
    // Combined filter tests
    test_combined_date_and_value_filter();
    test_combined_include_and_exclude_filter();
    test_combined_unique_and_value_filter();
    
    // Thread safety tests
    test_unique_rows_thread_safety();
    
    // Bulk setter tests
    test_bulk_setters();
    
    std::cout << "\nAll ReadingFilter tests passed!" << std::endl;
    return 0;
}
