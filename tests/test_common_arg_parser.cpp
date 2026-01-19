#include "../include/common_arg_parser.h"
#include <cassert>
#include <iostream>
#include <cstring>
#include <vector>

// Helper to create argv from strings
std::vector<char*> make_argv(std::vector<std::string>& args) {
    std::vector<char*> argv;
    for (auto& arg : args) {
        argv.push_back(&arg[0]);
    }
    return argv;
}

void test_default_values() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getRecursive() == false);
    assert(parser.getVerbosity() == 0);
    assert(parser.getInputFormat() == "auto");
    assert(parser.getMaxDepth() == -1);
    assert(parser.getMinDate() == 0);
    assert(parser.getMaxDate() == 0);
    assert(parser.getTailLines() == 0);
    assert(parser.getInputFiles().empty());
    
    std::cout << "[PASS] test_default_values" << std::endl;
}

void test_recursive_flag() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "-r"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getRecursive() == true);
    
    std::cout << "[PASS] test_recursive_flag" << std::endl;
}

void test_recursive_long_flag() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "--recursive"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getRecursive() == true);
    
    std::cout << "[PASS] test_recursive_long_flag" << std::endl;
}

void test_verbosity_v() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "-v"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getVerbosity() == 1);
    
    std::cout << "[PASS] test_verbosity_v" << std::endl;
}

void test_verbosity_V() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "-V"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getVerbosity() == 2);
    
    std::cout << "[PASS] test_verbosity_V" << std::endl;
}

void test_input_format_json() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "-if", "json"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getInputFormat() == "json");
    
    std::cout << "[PASS] test_input_format_json" << std::endl;
}

void test_input_format_csv() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "-if", "csv"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getInputFormat() == "csv");
    
    std::cout << "[PASS] test_input_format_csv" << std::endl;
}

void test_input_format_case_insensitive() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "-if", "CSV"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getInputFormat() == "csv");
    
    std::cout << "[PASS] test_input_format_case_insensitive" << std::endl;
}

void test_input_format_long_flag() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "--input-format", "csv"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getInputFormat() == "csv");
    
    std::cout << "[PASS] test_input_format_long_flag" << std::endl;
}

void test_extension_filter() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "-e", ".out"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getExtensionFilter() == ".out");
    
    std::cout << "[PASS] test_extension_filter" << std::endl;
}

void test_extension_filter_adds_dot() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "-e", "out"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getExtensionFilter() == ".out");
    
    std::cout << "[PASS] test_extension_filter_adds_dot" << std::endl;
}

void test_max_depth() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "-d", "3"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getMaxDepth() == 3);
    
    std::cout << "[PASS] test_max_depth" << std::endl;
}

void test_max_depth_long() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "--depth", "5"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getMaxDepth() == 5);
    
    std::cout << "[PASS] test_max_depth_long" << std::endl;
}

void test_tail_lines() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "--tail", "100"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getTailLines() == 100);
    
    std::cout << "[PASS] test_tail_lines" << std::endl;
}

void test_min_date_unix() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "--min-date", "1700000000"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getMinDate() == 1700000000);
    
    std::cout << "[PASS] test_min_date_unix" << std::endl;
}

void test_max_date_unix() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "--max-date", "1700000000"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    // maxDate uses parseDateEndOfDay which adds time for end of day
    // For a Unix timestamp, it should be returned as-is
    assert(parser.getMaxDate() == 1700000000);
    
    std::cout << "[PASS] test_max_date_unix" << std::endl;
}

void test_remove_errors_flag() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "--remove-errors"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getRemoveErrors() == true);
    
    std::cout << "[PASS] test_remove_errors_flag" << std::endl;
}

void test_remove_empty_json_flag() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "--remove-empty-json"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getRemoveEmptyJson() == true);
    
    std::cout << "[PASS] test_remove_empty_json_flag" << std::endl;
}

void test_clean_flag() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "--clean"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getRemoveEmptyJson() == true);
    assert(parser.getRemoveErrors() == true);
    assert(parser.getNotEmptyColumns().count("value") > 0);
    
    std::cout << "[PASS] test_clean_flag" << std::endl;
}

void test_not_empty_column() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "--not-empty", "temperature"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getNotEmptyColumns().count("temperature") > 0);
    
    std::cout << "[PASS] test_not_empty_column" << std::endl;
}

void test_multiple_not_empty() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "--not-empty", "temp", "--not-empty", "humidity"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getNotEmptyColumns().size() == 2);
    assert(parser.getNotEmptyColumns().count("temp") > 0);
    assert(parser.getNotEmptyColumns().count("humidity") > 0);
    
    std::cout << "[PASS] test_multiple_not_empty" << std::endl;
}

void test_only_value_filter() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "--only-value", "sensor:ds18b20"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    auto filters = parser.getOnlyValueFilters();
    assert(filters.count("sensor") > 0);
    assert(filters.at("sensor").count("ds18b20") > 0);
    
    std::cout << "[PASS] test_only_value_filter" << std::endl;
}

void test_exclude_value_filter() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "--exclude-value", "sensor:error"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    auto filters = parser.getExcludeValueFilters();
    assert(filters.count("sensor") > 0);
    assert(filters.at("sensor").count("error") > 0);
    
    std::cout << "[PASS] test_exclude_value_filter" << std::endl;
}

void test_combined_flags() {
    CommonArgParser parser;
    std::vector<std::string> args = {"program", "-r", "-v", "-e", ".out", "-d", "2", "--tail", "50"};
    auto argv = make_argv(args);
    
    bool result = parser.parse(static_cast<int>(argv.size()), argv.data());
    assert(result == true);
    assert(parser.getRecursive() == true);
    assert(parser.getVerbosity() == 1);
    assert(parser.getExtensionFilter() == ".out");
    assert(parser.getMaxDepth() == 2);
    assert(parser.getTailLines() == 50);
    
    std::cout << "[PASS] test_combined_flags" << std::endl;
}

void test_check_unknown_options_valid() {
    std::vector<std::string> args = {"program", "-r", "-v", "-e", ".out"};
    auto argv = make_argv(args);
    
    std::string unknown = CommonArgParser::checkUnknownOptions(
        static_cast<int>(argv.size()), argv.data());
    assert(unknown.empty());
    
    std::cout << "[PASS] test_check_unknown_options_valid" << std::endl;
}

void test_check_unknown_options_finds_unknown() {
    std::vector<std::string> args = {"program", "-r", "--unknown-flag"};
    auto argv = make_argv(args);
    
    std::string unknown = CommonArgParser::checkUnknownOptions(
        static_cast<int>(argv.size()), argv.data());
    assert(unknown == "--unknown-flag");
    
    std::cout << "[PASS] test_check_unknown_options_finds_unknown" << std::endl;
}

void test_check_unknown_options_with_additional() {
    std::vector<std::string> args = {"program", "-r", "--custom-flag"};
    auto argv = make_argv(args);
    
    std::set<std::string> additional = {"--custom-flag"};
    std::string unknown = CommonArgParser::checkUnknownOptions(
        static_cast<int>(argv.size()), argv.data(), additional);
    assert(unknown.empty());
    
    std::cout << "[PASS] test_check_unknown_options_with_additional" << std::endl;
}

int main() {
    std::cout << "================================" << std::endl;
    std::cout << "CommonArgParser Unit Tests" << std::endl;
    std::cout << "================================" << std::endl;
    
    // Default values
    test_default_values();
    
    // Flags
    test_recursive_flag();
    test_recursive_long_flag();
    test_verbosity_v();
    test_verbosity_V();
    
    // Input format
    test_input_format_json();
    test_input_format_csv();
    test_input_format_case_insensitive();
    test_input_format_long_flag();
    
    // Extension filter
    test_extension_filter();
    test_extension_filter_adds_dot();
    
    // Depth
    test_max_depth();
    test_max_depth_long();
    
    // Tail
    test_tail_lines();
    
    // Date filtering
    test_min_date_unix();
    test_max_date_unix();
    
    // Filtering flags
    test_remove_errors_flag();
    test_remove_empty_json_flag();
    test_clean_flag();
    test_not_empty_column();
    test_multiple_not_empty();
    test_only_value_filter();
    test_exclude_value_filter();
    
    // Combined
    test_combined_flags();
    
    // Unknown options check
    test_check_unknown_options_valid();
    test_check_unknown_options_finds_unknown();
    test_check_unknown_options_with_additional();
    
    std::cout << "================================" << std::endl;
    std::cout << "All CommonArgParser tests passed!" << std::endl;
    std::cout << "================================" << std::endl;
    
    return 0;
}
