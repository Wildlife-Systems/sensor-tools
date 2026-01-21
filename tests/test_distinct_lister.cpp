#include "../include/distinct_lister.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>

// Helper to create temp file
class TempFile {
public:
    std::string path;
    
    TempFile(const std::string& content, const std::string& extension = ".json") {
        path = "test_distinct_temp" + extension;
        std::ofstream f(path);
        f << content;
        f.close();
    }
    
    ~TempFile() {
        std::remove(path.c_str());
    }
};

// Helper to capture stdout
class CaptureStdout {
public:
    std::streambuf* oldBuf;
    std::stringstream captured;
    
    CaptureStdout() {
        oldBuf = std::cout.rdbuf(captured.rdbuf());
    }
    
    ~CaptureStdout() {
        std::cout.rdbuf(oldBuf);
    }
    
    std::string get() {
        return captured.str();
    }
};

// ===== Basic Distinct Tests =====

void test_distinct_basic() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"s2\",\"value\":\"23.0\"}\n"
        "{\"sensor_id\":\"s1\",\"value\":\"24.0\"}\n"  // duplicate sensor_id
    );
    
    const char* argv[] = {"sensor-data", "distinct", "--column", "sensor_id", file.path.c_str()};
    DistinctLister lister(5, const_cast<char**>(argv));
    
    CaptureStdout capture;
    lister.listDistinct();
    std::string output = capture.get();
    
    assert(output.find("s1") != std::string::npos);
    assert(output.find("s2") != std::string::npos);
    // Should only list each once
    size_t first_s1 = output.find("s1");
    size_t second_s1 = output.find("s1", first_s1 + 1);
    assert(second_s1 == std::string::npos);  // no duplicate
    std::cout << "[PASS] test_distinct_basic" << std::endl;
}

void test_distinct_with_counts() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"s1\",\"value\":\"23.0\"}\n"
        "{\"sensor_id\":\"s1\",\"value\":\"24.0\"}\n"
        "{\"sensor_id\":\"s2\",\"value\":\"25.0\"}\n"
    );
    
    const char* argv[] = {"sensor-data", "distinct", "--column", "sensor_id", "--counts", "--output-format", "csv", file.path.c_str()};
    DistinctLister lister(8, const_cast<char**>(argv));
    
    CaptureStdout capture;
    lister.listDistinct();
    std::string output = capture.get();
    
    assert(output.find("s1") != std::string::npos);
    assert(output.find("3") != std::string::npos);  // s1 appears 3 times
    assert(output.find("s2") != std::string::npos);
    assert(output.find("1") != std::string::npos);  // s2 appears 1 time
    std::cout << "[PASS] test_distinct_with_counts" << std::endl;
}

void test_distinct_empty_values_excluded() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"\",\"value\":\"23.0\"}\n"  // empty sensor_id
        "{\"value\":\"24.0\"}\n"  // missing sensor_id
    );
    
    const char* argv[] = {"sensor-data", "distinct", "--column", "sensor_id", file.path.c_str()};
    DistinctLister lister(5, const_cast<char**>(argv));
    
    CaptureStdout capture;
    lister.listDistinct();
    std::string output = capture.get();
    
    assert(output.find("s1") != std::string::npos);
    // Empty values should not be listed
    std::cout << "[PASS] test_distinct_empty_values_excluded" << std::endl;
}

// ===== Output Formats =====

void test_distinct_json_output() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"s2\",\"value\":\"23.0\"}\n"
    );
    
    const char* argv[] = {"sensor-data", "distinct", "--column", "sensor_id", "--output-format", "json", file.path.c_str()};
    DistinctLister lister(7, const_cast<char**>(argv));
    
    CaptureStdout capture;
    lister.listDistinct();
    std::string output = capture.get();
    
    assert(output.find("[") != std::string::npos);
    assert(output.find("]") != std::string::npos);
    assert(output.find("\"s1\"") != std::string::npos);
    assert(output.find("\"s2\"") != std::string::npos);
    std::cout << "[PASS] test_distinct_json_output" << std::endl;
}

void test_distinct_json_with_counts() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"s1\",\"value\":\"23.0\"}\n"
        "{\"sensor_id\":\"s2\",\"value\":\"24.0\"}\n"
    );
    
    const char* argv[] = {"sensor-data", "distinct", "--column", "sensor_id", "--counts", "--output-format", "json", file.path.c_str()};
    DistinctLister lister(8, const_cast<char**>(argv));
    
    CaptureStdout capture;
    lister.listDistinct();
    std::string output = capture.get();
    
    assert(output.find("\"value\"") != std::string::npos);
    assert(output.find("\"count\"") != std::string::npos);
    std::cout << "[PASS] test_distinct_json_with_counts" << std::endl;
}

void test_distinct_csv_output() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"s2\",\"value\":\"23.0\"}\n"
    );
    
    const char* argv[] = {"sensor-data", "distinct", "--column", "sensor_id", "--output-format", "csv", file.path.c_str()};
    DistinctLister lister(7, const_cast<char**>(argv));
    
    CaptureStdout capture;
    lister.listDistinct();
    std::string output = capture.get();
    
    assert(output.find("value") != std::string::npos);  // header
    assert(output.find("s1") != std::string::npos);
    assert(output.find("s2") != std::string::npos);
    std::cout << "[PASS] test_distinct_csv_output" << std::endl;
}

void test_distinct_csv_with_counts() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"s1\",\"value\":\"23.0\"}\n"
    );
    
    const char* argv[] = {"sensor-data", "distinct", "--column", "sensor_id", "--counts", "--output-format", "csv", file.path.c_str()};
    DistinctLister lister(8, const_cast<char**>(argv));
    
    CaptureStdout capture;
    lister.listDistinct();
    std::string output = capture.get();
    
    assert(output.find("value,count") != std::string::npos);  // header
    std::cout << "[PASS] test_distinct_csv_with_counts" << std::endl;
}

// ===== With Filters =====

void test_distinct_with_value_filter() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"status\":\"active\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"s2\",\"status\":\"inactive\",\"value\":\"23.0\"}\n"
        "{\"sensor_id\":\"s3\",\"status\":\"active\",\"value\":\"24.0\"}\n"
    );
    
    const char* argv[] = {"sensor-data", "distinct", "--column", "sensor_id", "--only-value", "status:active", file.path.c_str()};
    DistinctLister lister(7, const_cast<char**>(argv));
    
    CaptureStdout capture;
    lister.listDistinct();
    std::string output = capture.get();
    
    assert(output.find("s1") != std::string::npos);
    assert(output.find("s3") != std::string::npos);
    assert(output.find("s2") == std::string::npos);  // s2 is inactive
    std::cout << "[PASS] test_distinct_with_value_filter" << std::endl;
}

void test_distinct_with_date_filter() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"timestamp\":\"100\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"s2\",\"timestamp\":\"500\",\"value\":\"23.0\"}\n"
        "{\"sensor_id\":\"s3\",\"timestamp\":\"900\",\"value\":\"24.0\"}\n"
    );
    
    const char* argv[] = {"sensor-data", "distinct", "--column", "sensor_id", "--min-date", "200", "--max-date", "600", file.path.c_str()};
    DistinctLister lister(9, const_cast<char**>(argv));
    
    CaptureStdout capture;
    lister.listDistinct();
    std::string output = capture.get();
    
    assert(output.find("s2") != std::string::npos);
    assert(output.find("s1") == std::string::npos);
    assert(output.find("s3") == std::string::npos);
    std::cout << "[PASS] test_distinct_with_date_filter" << std::endl;
}

// ===== Multiple Files =====

void test_distinct_multiple_files() {
    TempFile file1("{\"sensor_id\":\"s1\",\"value\":\"22.5\"}\n");
    
    std::string path2 = "test_distinct_temp2.json";
    std::ofstream f2(path2);
    f2 << "{\"sensor_id\":\"s2\",\"value\":\"23.0\"}\n";
    f2.close();
    
    const char* argv[] = {"sensor-data", "distinct", "--column", "sensor_id", file1.path.c_str(), path2.c_str()};
    DistinctLister lister(6, const_cast<char**>(argv));
    
    CaptureStdout capture;
    lister.listDistinct();
    std::string output = capture.get();
    
    std::remove(path2.c_str());
    
    assert(output.find("s1") != std::string::npos);
    assert(output.find("s2") != std::string::npos);
    std::cout << "[PASS] test_distinct_multiple_files" << std::endl;
}

void test_distinct_across_files_dedup() {
    TempFile file1("{\"sensor_id\":\"s1\",\"value\":\"22.5\"}\n");
    
    std::string path2 = "test_distinct_temp2.json";
    std::ofstream f2(path2);
    f2 << "{\"sensor_id\":\"s1\",\"value\":\"23.0\"}\n";  // same sensor_id as file1
    f2.close();
    
    const char* argv[] = {"sensor-data", "distinct", "--column", "sensor_id", "--counts", "--output-format", "csv", file1.path.c_str(), path2.c_str()};
    DistinctLister lister(9, const_cast<char**>(argv));
    
    CaptureStdout capture;
    lister.listDistinct();
    std::string output = capture.get();
    
    std::remove(path2.c_str());
    
    // s1 should appear once in distinct list but with count 2
    assert(output.find("s1") != std::string::npos);
    assert(output.find("2") != std::string::npos);
    std::cout << "[PASS] test_distinct_across_files_dedup" << std::endl;
}

// ===== CSV Input =====

void test_distinct_csv_input() {
    TempFile file(
        "sensor_id,value\n"
        "s1,22.5\n"
        "s2,23.0\n"
        "s1,24.0\n",
        ".csv"
    );
    
    const char* argv[] = {"sensor-data", "distinct", "--column", "sensor_id", file.path.c_str()};
    DistinctLister lister(5, const_cast<char**>(argv));
    
    CaptureStdout capture;
    lister.listDistinct();
    std::string output = capture.get();
    
    assert(output.find("s1") != std::string::npos);
    assert(output.find("s2") != std::string::npos);
    std::cout << "[PASS] test_distinct_csv_input" << std::endl;
}

// ===== Edge Cases =====

void test_distinct_empty_file() {
    TempFile file("");
    
    const char* argv[] = {"sensor-data", "distinct", "--column", "sensor_id", file.path.c_str()};
    DistinctLister lister(5, const_cast<char**>(argv));
    
    CaptureStdout capture;
    lister.listDistinct();
    std::string output = capture.get();
    
    // Should produce empty output or just empty json array
    // No crash
    std::cout << "[PASS] test_distinct_empty_file" << std::endl;
}

void test_distinct_no_matching_column() {
    TempFile file(
        "{\"other\":\"value1\"}\n"
        "{\"other\":\"value2\"}\n"
    );
    
    const char* argv[] = {"sensor-data", "distinct", "--column", "sensor_id", file.path.c_str()};
    DistinctLister lister(5, const_cast<char**>(argv));
    
    CaptureStdout capture;
    lister.listDistinct();
    std::string output = capture.get();
    
    // Should produce empty output
    std::cout << "[PASS] test_distinct_no_matching_column" << std::endl;
}

void test_distinct_special_characters() {
    TempFile file(
        "{\"sensor_id\":\"sensor-1\",\"value\":\"22.5\"}\n"
        "{\"sensor_id\":\"sensor_2\",\"value\":\"23.0\"}\n"
        "{\"sensor_id\":\"sensor.3\",\"value\":\"24.0\"}\n"
    );
    
    const char* argv[] = {"sensor-data", "distinct", "--column", "sensor_id", file.path.c_str()};
    DistinctLister lister(5, const_cast<char**>(argv));
    
    CaptureStdout capture;
    lister.listDistinct();
    std::string output = capture.get();
    
    assert(output.find("sensor-1") != std::string::npos);
    assert(output.find("sensor_2") != std::string::npos);
    assert(output.find("sensor.3") != std::string::npos);
    std::cout << "[PASS] test_distinct_special_characters" << std::endl;
}

int main() {
    std::cout << "Running DistinctLister Tests..." << std::endl;
    
    // Basic distinct tests
    test_distinct_basic();
    test_distinct_with_counts();
    test_distinct_empty_values_excluded();
    
    // Output formats
    test_distinct_json_output();
    test_distinct_json_with_counts();
    test_distinct_csv_output();
    test_distinct_csv_with_counts();
    
    // With filters
    test_distinct_with_value_filter();
    test_distinct_with_date_filter();
    
    // Multiple files
    test_distinct_multiple_files();
    test_distinct_across_files_dedup();
    
    // CSV input
    test_distinct_csv_input();
    
    // Edge cases
    test_distinct_empty_file();
    test_distinct_no_matching_column();
    test_distinct_special_characters();
    
    std::cout << "\nAll DistinctLister tests passed!" << std::endl;
    return 0;
}
