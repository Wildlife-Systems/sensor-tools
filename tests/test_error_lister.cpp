#include "../include/error_lister.h"
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
        path = "test_error_lister_temp" + extension;
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

// ===== Basic List Errors Tests =====

void test_list_errors_single() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"temperature\":\"22.5\"}\n"
        "{\"sensor_id\":\"s2\",\"temperature\":\"85\"}\n"  // error reading
        "{\"sensor_id\":\"s3\",\"temperature\":\"24.0\"}\n"
    );
    
    const char* argv[] = {"sensor-data", "list-errors", file.path.c_str()};
    
    CaptureStdout capture;
    ErrorLister lister(3, const_cast<char**>(argv)); lister.listErrors();
    std::string output = capture.get();
    
    // Should output the error reading
    assert(output.find("s2") != std::string::npos);
    assert(output.find("85") != std::string::npos);
    std::cout << "[PASS] test_list_errors_single" << std::endl;
}

void test_list_errors_multiple() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"temperature\":\"85\"}\n"    // error
        "{\"sensor_id\":\"s2\",\"temperature\":\"22.5\"}\n"  // normal
        "{\"sensor_id\":\"s3\",\"temperature\":\"-127\"}\n"  // error
    );
    
    const char* argv[] = {"sensor-data", "list-errors", file.path.c_str()};
    
    CaptureStdout capture;
    ErrorLister lister(3, const_cast<char**>(argv)); lister.listErrors();
    std::string output = capture.get();
    
    // Should output both error readings
    assert(output.find("s1") != std::string::npos);
    assert(output.find("s3") != std::string::npos);
    assert(output.find("s2") == std::string::npos);  // normal reading
    std::cout << "[PASS] test_list_errors_multiple" << std::endl;
}

void test_list_errors_no_errors() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"temperature\":\"22.5\"}\n"
        "{\"sensor_id\":\"s2\",\"temperature\":\"23.0\"}\n"
    );
    
    const char* argv[] = {"sensor-data", "list-errors", file.path.c_str()};
    
    CaptureStdout capture;
    ErrorLister lister(3, const_cast<char**>(argv)); lister.listErrors();
    std::string output = capture.get();
    
    // Should produce minimal output (no errors)
    assert(output.find("s1") == std::string::npos);
    assert(output.find("s2") == std::string::npos);
    std::cout << "[PASS] test_list_errors_no_errors" << std::endl;
}

// ===== Different Error Types =====

void test_list_errors_temperature_85() {
    TempFile file("{\"sensor_id\":\"s1\",\"temperature\":\"85\"}\n");
    
    const char* argv[] = {"sensor-data", "list-errors", file.path.c_str()};
    
    CaptureStdout capture;
    ErrorLister lister(3, const_cast<char**>(argv)); lister.listErrors();
    std::string output = capture.get();
    
    assert(output.find("s1") != std::string::npos);
    std::cout << "[PASS] test_list_errors_temperature_85" << std::endl;
}

void test_list_errors_temperature_minus127() {
    TempFile file("{\"sensor_id\":\"s1\",\"temperature\":\"-127\"}\n");
    
    const char* argv[] = {"sensor-data", "list-errors", file.path.c_str()};
    
    CaptureStdout capture;
    ErrorLister lister(3, const_cast<char**>(argv)); lister.listErrors();
    std::string output = capture.get();
    
    assert(output.find("s1") != std::string::npos);
    std::cout << "[PASS] test_list_errors_temperature_minus127" << std::endl;
}

void test_list_errors_value_error() {
    TempFile file("{\"sensor_id\":\"s1\",\"value\":\"Error\"}\n");
    
    const char* argv[] = {"sensor-data", "list-errors", file.path.c_str()};
    
    CaptureStdout capture;
    ErrorLister lister(3, const_cast<char**>(argv)); lister.listErrors();
    std::string output = capture.get();
    
    // "Error" in value field is an error condition
    assert(output.find("s1") != std::string::npos);
    std::cout << "[PASS] test_list_errors_value_error" << std::endl;
}

// ===== With Filters =====

void test_list_errors_with_date_filter() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"timestamp\":\"100\",\"temperature\":\"85\"}\n"  // early error
        "{\"sensor_id\":\"s2\",\"timestamp\":\"500\",\"temperature\":\"85\"}\n"  // late error
    );
    
    const char* argv[] = {"sensor-data", "list-errors", "--min-date", "200", file.path.c_str()};
    
    CaptureStdout capture;
    ErrorLister lister(5, const_cast<char**>(argv)); lister.listErrors();
    std::string output = capture.get();
    
    // Only s2 should be listed (after 200)
    assert(output.find("s2") != std::string::npos);
    assert(output.find("s1") == std::string::npos);
    std::cout << "[PASS] test_list_errors_with_date_filter" << std::endl;
}

void test_list_errors_with_sensor_filter() {
    TempFile file(
        "{\"sensor_id\":\"s1\",\"temperature\":\"85\"}\n"
        "{\"sensor_id\":\"s2\",\"temperature\":\"85\"}\n"
    );
    
    const char* argv[] = {"sensor-data", "list-errors", "--only-value", "sensor_id:s1", file.path.c_str()};
    
    CaptureStdout capture;
    ErrorLister lister(5, const_cast<char**>(argv)); lister.listErrors();
    std::string output = capture.get();
    
    assert(output.find("s1") != std::string::npos);
    assert(output.find("s2") == std::string::npos);
    std::cout << "[PASS] test_list_errors_with_sensor_filter" << std::endl;
}

// ===== Multiple Files =====

void test_list_errors_multiple_files() {
    TempFile file1("{\"sensor_id\":\"s1\",\"temperature\":\"85\"}\n");
    
    std::string path2 = "test_error_lister_temp2.json";
    std::ofstream f2(path2);
    f2 << "{\"sensor_id\":\"s2\",\"temperature\":\"85\"}\n";
    f2.close();
    
    const char* argv[] = {"sensor-data", "list-errors", file1.path.c_str(), path2.c_str()};
    
    CaptureStdout capture;
    ErrorLister lister(4, const_cast<char**>(argv)); lister.listErrors();
    std::string output = capture.get();
    
    std::remove(path2.c_str());
    
    assert(output.find("s1") != std::string::npos);
    assert(output.find("s2") != std::string::npos);
    std::cout << "[PASS] test_list_errors_multiple_files" << std::endl;
}

// ===== CSV Input =====

void test_list_errors_csv_input() {
    TempFile file(
        "sensor_id,temperature\n"
        "s1,85\n"
        "s2,22.5\n",
        ".csv"
    );
    
    const char* argv[] = {"sensor-data", "list-errors", file.path.c_str()};
    
    CaptureStdout capture;
    ErrorLister lister(3, const_cast<char**>(argv)); lister.listErrors();
    std::string output = capture.get();
    
    assert(output.find("s1") != std::string::npos);
    assert(output.find("s2") == std::string::npos);
    std::cout << "[PASS] test_list_errors_csv_input" << std::endl;
}

// ===== Edge Cases =====

void test_list_errors_empty_file() {
    TempFile file("");
    
    const char* argv[] = {"sensor-data", "list-errors", file.path.c_str()};
    
    CaptureStdout capture;
    ErrorLister lister(3, const_cast<char**>(argv)); lister.listErrors();
    std::string output = capture.get();
    
    // Should not crash
    std::cout << "[PASS] test_list_errors_empty_file" << std::endl;
}

int main() {
    std::cout << "Running ErrorLister Tests..." << std::endl;
    
    // Basic list errors tests
    test_list_errors_single();
    test_list_errors_multiple();
    test_list_errors_no_errors();
    
    // Different error types
    test_list_errors_temperature_85();
    test_list_errors_temperature_minus127();
    test_list_errors_value_error();
    
    // With filters
    test_list_errors_with_date_filter();
    test_list_errors_with_sensor_filter();
    
    // Multiple files
    test_list_errors_multiple_files();
    
    // CSV input
    test_list_errors_csv_input();
    
    // Edge cases
    test_list_errors_empty_file();
    
    std::cout << "\nAll ErrorLister tests passed!" << std::endl;
    return 0;
}
