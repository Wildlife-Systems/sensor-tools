#include "../include/data_reader.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>

// Helper to create temp file
class TempFile {
public:
    std::string path;
    
    TempFile(const std::string& content, const std::string& extension = ".txt") {
        path = "test_data_reader_temp" + extension;
        std::ofstream f(path);
        f << content;
        f.close();
    }
    
    ~TempFile() {
        std::remove(path.c_str());
    }
};

void test_process_json_file_basic() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"s2\",\"value\":\"23.0\"}\n"
    );
    
    DataReader reader;
    int count = 0;
    
    reader.processFile(file.path, [&](const Reading& reading, int, const std::string&) {
        count++;
        assert(!reading.empty());
    });
    
    assert(count == 2);
    std::cout << "[PASS] test_process_json_file_basic" << std::endl;
}

void test_process_csv_file_basic() {
    TempFile file(
        "sensor_id,value\n"
        "s1,22.5\n"
        "s2,23.0\n",
        ".csv"
    );
    
    DataReader reader;  // Uses default "auto" format which detects from extension
    int count = 0;
    
    reader.processFile(file.path, [&](const Reading& reading, int, const std::string&) {
        count++;
        assert(reading.count("sensor_id") > 0);
        assert(reading.count("value") > 0);
    });
    
    assert(count == 2);
    std::cout << "[PASS] test_process_csv_file_basic" << std::endl;
}

void test_process_json_with_date_filter() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"timestamp\":\"100\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"s2\",\"timestamp\":\"200\",\"value\":\"23.0\"}\n"
        "{\"sensor_id\":\"s3\",\"timestamp\":\"300\",\"value\":\"24.0\"}\n"
    );
    
    // Filter: minDate=150, maxDate=250 -> only s2 should pass
    DataReader reader(0, "json");
    reader.setDateRange(150, 250);
    int count = 0;
    std::string foundId;
    
    reader.processFile(file.path, [&](const Reading& reading, int, const std::string&) {
        count++;
        foundId = reading.at("sensor_id");
    });
    
    assert(count == 1);
    assert(foundId == "s2");
    std::cout << "[PASS] test_process_json_with_date_filter" << std::endl;
}

void test_process_json_with_min_date_only() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"timestamp\":\"100\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"s2\",\"timestamp\":\"200\",\"value\":\"23.0\"}\n"
    );
    
    // Filter: minDate=150 -> only s2 should pass
    DataReader reader(0, "json");
    reader.setDateRange(150, 0);
    int count = 0;
    
    reader.processFile(file.path, [&](const Reading&, int, const std::string&) {
        count++;
    });
    
    assert(count == 1);
    std::cout << "[PASS] test_process_json_with_min_date_only" << std::endl;
}

void test_process_json_with_max_date_only() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"timestamp\":\"100\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"s2\",\"timestamp\":\"200\",\"value\":\"23.0\"}\n"
    );
    
    // Filter: maxDate=150 -> only s1 should pass
    DataReader reader(0, "json");
    reader.setDateRange(0, 150);
    int count = 0;
    
    reader.processFile(file.path, [&](const Reading&, int, const std::string&) {
        count++;
    });
    
    assert(count == 1);
    std::cout << "[PASS] test_process_json_with_max_date_only" << std::endl;
}

void test_process_with_tail_lines() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"value\":\"1\"}\n"
        "{\"sensor_id\":\"s2\",\"value\":\"2\"}\n"
        "{\"sensor_id\":\"s3\",\"value\":\"3\"}\n"
        "{\"sensor_id\":\"s4\",\"value\":\"4\"}\n"
    );
    
    // Tail 2 lines - should only get s3 and s4
    DataReader reader(0, "json", 2);
    std::vector<std::string> ids;
    
    reader.processFile(file.path, [&](const Reading& reading, int, const std::string&) {
        ids.push_back(reading.at("sensor_id"));
    });
    
    assert(ids.size() == 2);
    assert(ids[0] == "s3");
    assert(ids[1] == "s4");
    std::cout << "[PASS] test_process_with_tail_lines" << std::endl;
}

void test_process_empty_file() {
    TempFile file("");
    
    DataReader reader;
    int count = 0;
    
    reader.processFile(file.path, [&](const Reading&, int, const std::string&) {
        count++;
    });
    
    assert(count == 0);
    std::cout << "[PASS] test_process_empty_file" << std::endl;
}

void test_process_json_array_format() {
    // JSON array with multiple readings per line
    TempFile file(
        "[{\"sensor_id\":\"s1\",\"value\":\"1\"},{\"sensor_id\":\"s2\",\"value\":\"2\"}]\n"
    );
    
    DataReader reader;
    int count = 0;
    
    reader.processFile(file.path, [&](const Reading&, int, const std::string&) {
        count++;
    });
    
    assert(count == 2);
    std::cout << "[PASS] test_process_json_array_format" << std::endl;
}

void test_process_stdin() {
    std::string input = "{\"sensor_id\":\"s1\",\"value\":\"22.5\"}\n";
    std::istringstream iss(input);
    std::streambuf* oldBuf = std::cin.rdbuf(iss.rdbuf());
    
    DataReader reader;
    int count = 0;
    
    reader.processStdin([&](const Reading& reading, int, const std::string&) {
        count++;
        assert(reading.at("sensor_id") == "s1");
    });
    
    std::cin.rdbuf(oldBuf);
    
    assert(count == 1);
    std::cout << "[PASS] test_process_stdin" << std::endl;
}

void test_process_csv_stdin() {
    std::string input = "sensor_id,value\ns1,22.5\n";
    std::istringstream iss(input);
    std::streambuf* oldBuf = std::cin.rdbuf(iss.rdbuf());
    
    DataReader reader(0, "csv");
    int count = 0;
    
    reader.processStdin([&](const Reading& reading, int, const std::string&) {
        count++;
        assert(reading.at("sensor_id") == "s1");
        assert(reading.at("value") == "22.5");
    });
    
    std::cin.rdbuf(oldBuf);
    
    assert(count == 1);
    std::cout << "[PASS] test_process_csv_stdin" << std::endl;
}

void test_reading_values_correct() {
    TempFile file(
        "{\"sensor_id\":\"test\",\"value\":\"123.456\",\"extra\":\"data\"}\n"
    );
    
    DataReader reader;
    
    reader.processFile(file.path, [&](const Reading& reading, int lineNum, const std::string& source) {
        assert(reading.at("sensor_id") == "test");
        assert(reading.at("value") == "123.456");
        assert(reading.at("extra") == "data");
        assert(lineNum == 1);
        assert(source.find("test_data_reader") != std::string::npos);
    });
    
    std::cout << "[PASS] test_reading_values_correct" << std::endl;
}

void test_line_numbers_correct() {
    TempFile file(
        "{\"id\":\"1\"}\n"
        "{\"id\":\"2\"}\n"
        "{\"id\":\"3\"}\n"
    );
    
    DataReader reader;
    std::vector<int> lineNums;
    
    reader.processFile(file.path, [&](const Reading&, int lineNum, const std::string&) {
        lineNums.push_back(lineNum);
    });
    
    assert(lineNums.size() == 3);
    assert(lineNums[0] == 1);
    assert(lineNums[1] == 2);
    assert(lineNums[2] == 3);
    std::cout << "[PASS] test_line_numbers_correct" << std::endl;
}

void test_skips_empty_lines() {
    TempFile file(
        "{\"id\":\"1\"}\n"
        "\n"
        "{\"id\":\"2\"}\n"
        "\n"
        "\n"
        "{\"id\":\"3\"}\n"
    );
    
    DataReader reader;
    int count = 0;
    
    reader.processFile(file.path, [&](const Reading&, int, const std::string&) {
        count++;
    });
    
    assert(count == 3);
    std::cout << "[PASS] test_skips_empty_lines" << std::endl;
}

void test_nonexistent_file() {
    DataReader reader;
    int count = 0;
    
    // Should handle gracefully without crashing
    reader.processFile("nonexistent_file_12345.json", [&](const Reading&, int, const std::string&) {
        count++;
    });
    
    assert(count == 0);
    std::cout << "[PASS] test_nonexistent_file" << std::endl;
}

void test_format_auto_detection_json() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"value\":\"22.5\"}\n",
        ".json"
    );
    
    DataReader reader(0, "auto");  // auto format detection
    int count = 0;
    
    reader.processFile(file.path, [&](const Reading& reading, int, const std::string&) {
        count++;
        assert(reading.at("sensor_id") == "s1");
    });
    
    assert(count == 1);
    std::cout << "[PASS] test_format_auto_detection_json" << std::endl;
}

void test_format_auto_detection_csv() {
    TempFile file(
        "sensor_id,value\n"
        "s1,22.5\n",
        ".csv"
    );
    
    DataReader reader(0, "auto");  // auto format detection
    int count = 0;
    
    reader.processFile(file.path, [&](const Reading& reading, int, const std::string&) {
        count++;
        assert(reading.at("sensor_id") == "s1");
    });
    
    assert(count == 1);
    std::cout << "[PASS] test_format_auto_detection_csv" << std::endl;
}

void test_value_filter_include() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"status\":\"active\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"s2\",\"status\":\"inactive\",\"value\":\"23.0\"}\n"
        "{\"sensor_id\":\"s3\",\"status\":\"active\",\"value\":\"24.0\"}\n"
    );
    
    DataReader reader(0, "json");
    reader.addOnlyValueFilter("status", "active");
    
    int count = 0;
    reader.processFile(file.path, [&](const Reading&, int, const std::string&) {
        count++;
    });
    
    assert(count == 2);  // s1 and s3
    std::cout << "[PASS] test_value_filter_include" << std::endl;
}

void test_value_filter_exclude() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"status\":\"active\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"s2\",\"status\":\"error\",\"value\":\"23.0\"}\n"
        "{\"sensor_id\":\"s3\",\"status\":\"active\",\"value\":\"24.0\"}\n"
    );
    
    DataReader reader(0, "json");
    reader.addExcludeValueFilter("status", "error");
    
    int count = 0;
    reader.processFile(file.path, [&](const Reading&, int, const std::string&) {
        count++;
    });
    
    assert(count == 2);  // s1 and s3
    std::cout << "[PASS] test_value_filter_exclude" << std::endl;
}

void test_not_empty_filter() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"s2\",\"value\":\"\"}\n"
        "{\"sensor_id\":\"s3\"}\n"  // missing value
    );
    
    DataReader reader(0, "json");
    reader.addNotEmptyColumn("value");
    
    int count = 0;
    reader.processFile(file.path, [&](const Reading&, int, const std::string&) {
        count++;
    });
    
    assert(count == 1);  // only s1
    std::cout << "[PASS] test_not_empty_filter" << std::endl;
}

void test_remove_errors_filter() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"temperature\":\"22.5\"}\n"
        "{\"sensor_id\":\"s2\",\"temperature\":\"85\"}\n"  // error
        "{\"sensor_id\":\"s3\",\"temperature\":\"24.0\"}\n"
    );
    
    DataReader reader(0, "json");
    reader.setRemoveErrors(true);
    
    int count = 0;
    reader.processFile(file.path, [&](const Reading&, int, const std::string&) {
        count++;
    });
    
    assert(count == 2);  // s1 and s3, not s2
    std::cout << "[PASS] test_remove_errors_filter" << std::endl;
}

void test_unique_rows_filter() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"s1\",\"value\":\"22.5\"}\n"  // duplicate
        "{\"sensor_id\":\"s2\",\"value\":\"23.0\"}\n"
    );
    
    DataReader reader(0, "json");
    reader.setUniqueRows(true);
    
    int count = 0;
    reader.processFile(file.path, [&](const Reading&, int, const std::string&) {
        count++;
    });
    
    assert(count == 2);  // s1 and s2, duplicate skipped
    std::cout << "[PASS] test_unique_rows_filter" << std::endl;
}

void test_combined_filters() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"timestamp\":\"500\",\"status\":\"active\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"s2\",\"timestamp\":\"500\",\"status\":\"inactive\",\"value\":\"23.0\"}\n"
        "{\"sensor_id\":\"s3\",\"timestamp\":\"100\",\"status\":\"active\",\"value\":\"24.0\"}\n"
    );
    
    DataReader reader(0, "json");
    reader.setDateRange(200, 0);  // after 200
    reader.addOnlyValueFilter("status", "active");
    
    int count = 0;
    reader.processFile(file.path, [&](const Reading&, int, const std::string&) {
        count++;
    });
    
    assert(count == 1);  // only s1 (active and timestamp 500)
    std::cout << "[PASS] test_combined_filters" << std::endl;
}

int main() {
    std::cout << "================================" << std::endl;
    std::cout << "DataReader Unit Tests" << std::endl;
    std::cout << "================================" << std::endl;
    
    // Basic file processing
    test_process_json_file_basic();
    test_process_csv_file_basic();
    test_process_empty_file();
    test_process_json_array_format();
    
    // Date filtering
    test_process_json_with_date_filter();
    test_process_json_with_min_date_only();
    test_process_json_with_max_date_only();
    
    // Tail lines
    test_process_with_tail_lines();
    
    // Stdin processing
    test_process_stdin();
    test_process_csv_stdin();
    
    // Value correctness
    test_reading_values_correct();
    test_line_numbers_correct();
    test_skips_empty_lines();
    
    // Error handling
    test_nonexistent_file();
    
    // Format detection
    test_format_auto_detection_json();
    test_format_auto_detection_csv();
    
    // Value filters
    test_value_filter_include();
    test_value_filter_exclude();
    test_not_empty_filter();
    test_remove_errors_filter();
    test_unique_rows_filter();
    test_combined_filters();
    
    std::cout << "================================" << std::endl;
    std::cout << "All DataReader tests passed!" << std::endl;
    std::cout << "================================" << std::endl;
    
    return 0;
}
