#include "../include/file_utils.h"
#include <cassert>
#include <iostream>
#include <fstream>

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

int main() {
    std::cout << "Running File Utils Tests..." << std::endl;
    test_is_csv_file();
    test_matches_extension();
    std::cout << "All File Utils tests passed!" << std::endl;
    return 0;
}
