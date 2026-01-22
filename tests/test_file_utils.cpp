#include "../include/file_utils.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <vector>

// Helper to create temp file
class TempFile {
public:
    std::string path;
    
    TempFile(const std::string& content, const std::string& extension = ".txt") {
        path = "test_file_utils_temp" + extension;
        std::ofstream f(path, std::ios::binary);  // Binary mode for exact control
        f << content;
        f.close();
    }
    
    ~TempFile() {
        std::remove(path.c_str());
    }
};

void test_is_csv_file() {
    assert(FileUtils::isCsvFile("data.csv") == true);
    assert(FileUtils::isCsvFile("data.CSV") == true);
    assert(FileUtils::isCsvFile("data.txt") == false);
    assert(FileUtils::isCsvFile("data.json") == false);
    assert(FileUtils::isCsvFile("data") == false);
    std::cout << "[PASS] test_is_csv_file" << std::endl;
}

void test_matches_extension() {
    assert(FileUtils::matchesExtension("file.out", ".out") == true);
    assert(FileUtils::matchesExtension("file.txt", ".out") == false);
    assert(FileUtils::matchesExtension("file.out", "") == true);
    assert(FileUtils::matchesExtension("file", ".out") == false);
    std::cout << "[PASS] test_matches_extension" << std::endl;
}

void test_matches_extension_json() {
    assert(FileUtils::matchesExtension("sensors.json", ".json") == true);
    assert(FileUtils::matchesExtension("data.JSON", ".json") == false);  // Case sensitive
    assert(FileUtils::matchesExtension("data.json", ".JSON") == false);
    std::cout << "[PASS] test_matches_extension_json" << std::endl;
}

void test_matches_extension_no_dot() {
    // Extension filter should include dot
    assert(FileUtils::matchesExtension("file.csv", "csv") == false);
    assert(FileUtils::matchesExtension("file.csv", ".csv") == true);
    std::cout << "[PASS] test_matches_extension_no_dot" << std::endl;
}

void test_matches_extension_multiple_dots() {
    assert(FileUtils::matchesExtension("data.backup.out", ".out") == true);
    assert(FileUtils::matchesExtension("sensors.2026.01.17.out", ".out") == true);
    std::cout << "[PASS] test_matches_extension_multiple_dots" << std::endl;
}

void test_is_csv_file_path_with_slashes() {
    assert(FileUtils::isCsvFile("/path/to/data.csv") == true);
    assert(FileUtils::isCsvFile("C:\\path\\to\\data.csv") == true);
    assert(FileUtils::isCsvFile("/path/to/data.json") == false);
    std::cout << "[PASS] test_is_csv_file_path_with_slashes" << std::endl;
}

void test_is_csv_file_hidden() {
    assert(FileUtils::isCsvFile(".hidden.csv") == true);
    assert(FileUtils::isCsvFile(".csv") == true);
    std::cout << "[PASS] test_is_csv_file_hidden" << std::endl;
}

// ===== readLinesReverse Tests =====

void test_read_lines_reverse_basic() {
    TempFile file("line1\nline2\nline3\n");
    
    std::vector<std::string> lines;
    int count = FileUtils::readLinesReverse(file.path, [&](const std::string& line) {
        lines.push_back(line);
        return true;  // continue
    });
    
    assert(count == 3);
    assert(lines.size() == 3);
    assert(lines[0] == "line3");  // Last line first
    assert(lines[1] == "line2");
    assert(lines[2] == "line1");  // First line last
    std::cout << "[PASS] test_read_lines_reverse_basic" << std::endl;
}

void test_read_lines_reverse_no_trailing_newline() {
    TempFile file("line1\nline2\nline3");  // No trailing newline
    
    std::vector<std::string> lines;
    int count = FileUtils::readLinesReverse(file.path, [&](const std::string& line) {
        lines.push_back(line);
        return true;
    });
    
    assert(count == 3);
    assert(lines.size() == 3);
    assert(lines[0] == "line3");
    assert(lines[1] == "line2");
    assert(lines[2] == "line1");
    std::cout << "[PASS] test_read_lines_reverse_no_trailing_newline" << std::endl;
}

void test_read_lines_reverse_single_line() {
    TempFile file("only one line\n");
    
    std::vector<std::string> lines;
    int count = FileUtils::readLinesReverse(file.path, [&](const std::string& line) {
        lines.push_back(line);
        return true;
    });
    
    assert(count == 1);
    assert(lines.size() == 1);
    assert(lines[0] == "only one line");
    std::cout << "[PASS] test_read_lines_reverse_single_line" << std::endl;
}

void test_read_lines_reverse_empty_file() {
    TempFile file("");
    
    std::vector<std::string> lines;
    int count = FileUtils::readLinesReverse(file.path, [&](const std::string& line) {
        lines.push_back(line);
        return true;
    });
    
    assert(count == 0);
    assert(lines.size() == 0);
    std::cout << "[PASS] test_read_lines_reverse_empty_file" << std::endl;
}

void test_read_lines_reverse_stop_early() {
    TempFile file("line1\nline2\nline3\nline4\nline5\n");
    
    std::vector<std::string> lines;
    int count = FileUtils::readLinesReverse(file.path, [&](const std::string& line) {
        lines.push_back(line);
        return lines.size() < 2;  // Stop after 2 lines
    });
    
    assert(count == 2);
    assert(lines.size() == 2);
    assert(lines[0] == "line5");  // Last line
    assert(lines[1] == "line4");  // Second to last
    std::cout << "[PASS] test_read_lines_reverse_stop_early" << std::endl;
}

void test_read_lines_reverse_empty_lines() {
    TempFile file("line1\n\nline3\n\nline5\n");
    
    std::vector<std::string> lines;
    int count = FileUtils::readLinesReverse(file.path, [&](const std::string& line) {
        lines.push_back(line);
        return true;
    });
    
    // Empty lines are skipped (based on implementation)
    assert(count == 3);
    assert(lines[0] == "line5");
    assert(lines[1] == "line3");
    assert(lines[2] == "line1");
    std::cout << "[PASS] test_read_lines_reverse_empty_lines" << std::endl;
}

void test_read_lines_reverse_nonexistent_file() {
    std::vector<std::string> lines;
    int count = FileUtils::readLinesReverse("nonexistent_file_12345.txt", [&](const std::string& line) {
        lines.push_back(line);
        return true;
    });
    
    assert(count == 0);
    assert(lines.size() == 0);
    std::cout << "[PASS] test_read_lines_reverse_nonexistent_file" << std::endl;
}

void test_read_lines_reverse_with_special_chars() {
    TempFile file("{\"key\":\"value\"}\n[1,2,3]\ntest,data,csv\n");
    
    std::vector<std::string> lines;
    int count = FileUtils::readLinesReverse(file.path, [&](const std::string& line) {
        lines.push_back(line);
        return true;
    });
    
    assert(count == 3);
    assert(lines[0] == "test,data,csv");
    assert(lines[1] == "[1,2,3]");
    assert(lines[2] == "{\"key\":\"value\"}");
    std::cout << "[PASS] test_read_lines_reverse_with_special_chars" << std::endl;
}

void test_read_lines_reverse_long_lines() {
    std::string longLine(1000, 'x');
    TempFile file("short\n" + longLine + "\nanother\n");
    
    std::vector<std::string> lines;
    int count = FileUtils::readLinesReverse(file.path, [&](const std::string& line) {
        lines.push_back(line);
        return true;
    });
    
    assert(count == 3);
    assert(lines[0] == "another");
    assert(lines[1] == longLine);
    assert(lines[1].length() == 1000);
    assert(lines[2] == "short");
    std::cout << "[PASS] test_read_lines_reverse_long_lines" << std::endl;
}

int main() {
    std::cout << "Running File Utils Tests..." << std::endl;
    test_is_csv_file();
    test_matches_extension();
    test_matches_extension_json();
    test_matches_extension_no_dot();
    test_matches_extension_multiple_dots();
    test_is_csv_file_path_with_slashes();
    test_is_csv_file_hidden();
    
    // readLinesReverse tests
    test_read_lines_reverse_basic();
    test_read_lines_reverse_no_trailing_newline();
    test_read_lines_reverse_single_line();
    test_read_lines_reverse_empty_file();
    test_read_lines_reverse_stop_early();
    test_read_lines_reverse_empty_lines();
    test_read_lines_reverse_nonexistent_file();
    test_read_lines_reverse_with_special_chars();
    test_read_lines_reverse_long_lines();
    
    std::cout << "All File Utils tests passed!" << std::endl;
    return 0;
}
