#include "../include/file_utils.h"
#include <cassert>
#include <iostream>

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

int main() {
    std::cout << "Running File Utils Tests..." << std::endl;
    test_is_csv_file();
    test_matches_extension();
    test_matches_extension_json();
    test_matches_extension_no_dot();
    test_matches_extension_multiple_dots();
    test_is_csv_file_path_with_slashes();
    test_is_csv_file_hidden();
    std::cout << "All File Utils tests passed!" << std::endl;
    return 0;
}
