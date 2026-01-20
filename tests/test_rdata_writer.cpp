#include "../include/rdata_writer.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>

// Get temp directory path (cross-platform)
std::string getTempDir() {
#ifdef _WIN32
    const char* tmp = std::getenv("TEMP");
    if (!tmp) tmp = std::getenv("TMP");
    if (!tmp) tmp = ".";
    return std::string(tmp) + "\\";
#else
    return "/tmp/";
#endif
}

// Helper to check if a file exists and has expected size range
bool fileExistsWithSize(const std::string& path, size_t minSize, size_t maxSize) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return false;
    size_t size = static_cast<size_t>(file.tellg());
    return size >= minSize && size <= maxSize;
}

// Helper to read file bytes for header verification
std::vector<char> readFileBytes(const std::string& path, size_t count) {
    std::ifstream file(path, std::ios::binary);
    std::vector<char> bytes(count);
    file.read(bytes.data(), count);
    return bytes;
}

void test_rdata_empty_data() {
    std::string filename = getTempDir() + "test_empty.RData";
    ReadingList readings;
    std::vector<std::string> headers = {"col1", "col2"};
    
    bool result = RDataWriter::writeRData(filename, readings, headers);
    assert(result == true);
    
    // File should exist with RData magic header
    auto bytes = readFileBytes(filename, 5);
    assert(bytes[0] == 'R');
    assert(bytes[1] == 'D');
    assert(bytes[2] == 'X');
    assert(bytes[3] == '2');
    assert(bytes[4] == '\n');
    
    std::remove(filename.c_str());
    std::cout << "[PASS] test_rdata_empty_data" << std::endl;
}

void test_rds_empty_data() {
    std::string filename = getTempDir() + "test_empty.rds";
    ReadingList readings;
    std::vector<std::string> headers = {"col1", "col2"};
    
    bool result = RDataWriter::writeRDS(filename, readings, headers);
    assert(result == true);
    
    // RDS files start with 'X\n' (no RDX2 magic)
    auto bytes = readFileBytes(filename, 2);
    assert(bytes[0] == 'X');
    assert(bytes[1] == '\n');
    
    std::remove(filename.c_str());
    std::cout << "[PASS] test_rds_empty_data" << std::endl;
}

void test_rdata_single_row() {
    std::string filename = getTempDir() + "test_single.RData";
    ReadingList readings;
    Reading r1;
    r1["sensor_id"] = "temp1";
    r1["value"] = "23.5";
    r1["unit"] = "C";
    readings.push_back(r1);
    
    std::vector<std::string> headers = {"sensor_id", "value", "unit"};
    
    bool result = RDataWriter::writeRData(filename, readings, headers);
    assert(result == true);
    
    // File should be reasonable size for small data frame
    assert(fileExistsWithSize(filename, 100, 500));
    
    std::remove(filename.c_str());
    std::cout << "[PASS] test_rdata_single_row" << std::endl;
}

void test_rdata_multiple_rows() {
    std::string filename = getTempDir() + "test_multi.RData";
    ReadingList readings;
    
    for (int i = 0; i < 10; i++) {
        Reading r;
        r["sensor_id"] = "sensor" + std::to_string(i);
        r["value"] = std::to_string(20.0 + i * 0.5);
        readings.push_back(r);
    }
    
    std::vector<std::string> headers = {"sensor_id", "value"};
    
    bool result = RDataWriter::writeRData(filename, readings, headers);
    assert(result == true);
    
    // Should be larger than single row
    assert(fileExistsWithSize(filename, 200, 1000));
    
    std::remove(filename.c_str());
    std::cout << "[PASS] test_rdata_multiple_rows" << std::endl;
}

void test_rdata_missing_columns() {
    std::string filename = getTempDir() + "test_missing.RData";
    ReadingList readings;
    
    Reading r1;
    r1["sensor_id"] = "temp1";
    r1["value"] = "23.5";
    // Missing "unit" column
    readings.push_back(r1);
    
    Reading r2;
    r2["sensor_id"] = "temp2";
    // Missing "value" column
    r2["unit"] = "C";
    readings.push_back(r2);
    
    std::vector<std::string> headers = {"sensor_id", "value", "unit"};
    
    bool result = RDataWriter::writeRData(filename, readings, headers);
    assert(result == true);
    
    std::remove(filename.c_str());
    std::cout << "[PASS] test_rdata_missing_columns" << std::endl;
}

void test_rdata_special_characters() {
    std::string filename = getTempDir() + "test_special.RData";
    ReadingList readings;
    
    Reading r1;
    r1["sensor_id"] = "temp,1";  // Comma
    r1["value"] = "23.5";
    readings.push_back(r1);
    
    Reading r2;
    r2["sensor_id"] = "temp\"2";  // Quote
    r2["value"] = "24.5";
    readings.push_back(r2);
    
    Reading r3;
    r3["sensor_id"] = "temp\n3";  // Newline
    r3["value"] = "25.5";
    readings.push_back(r3);
    
    std::vector<std::string> headers = {"sensor_id", "value"};
    
    bool result = RDataWriter::writeRData(filename, readings, headers);
    assert(result == true);
    
    std::remove(filename.c_str());
    std::cout << "[PASS] test_rdata_special_characters" << std::endl;
}

void test_rds_single_row() {
    std::string filename = getTempDir() + "test_single.rds";
    ReadingList readings;
    Reading r1;
    r1["sensor_id"] = "temp1";
    r1["value"] = "23.5";
    readings.push_back(r1);
    
    std::vector<std::string> headers = {"sensor_id", "value"};
    
    bool result = RDataWriter::writeRDS(filename, readings, headers);
    assert(result == true);
    
    // RDS should be slightly smaller than RData (no variable name wrapper)
    assert(fileExistsWithSize(filename, 80, 400));
    
    std::remove(filename.c_str());
    std::cout << "[PASS] test_rds_single_row" << std::endl;
}

void test_rdata_custom_table_name() {
    std::string filename = getTempDir() + "test_custom.RData";
    ReadingList readings;
    Reading r1;
    r1["x"] = "1";
    readings.push_back(r1);
    
    std::vector<std::string> headers = {"x"};
    
    bool result = RDataWriter::writeRData(filename, readings, headers, "my_custom_data");
    assert(result == true);
    
    std::remove(filename.c_str());
    std::cout << "[PASS] test_rdata_custom_table_name" << std::endl;
}

void test_rdata_invalid_path() {
#ifdef _WIN32
    std::string filename = "Z:\\nonexistent\\directory\\test.RData";
#else
    std::string filename = "/nonexistent/directory/test.RData";
#endif
    ReadingList readings;
    std::vector<std::string> headers = {"col1"};
    
    // Should fail gracefully for invalid path
    bool result = RDataWriter::writeRData(filename, readings, headers);
    assert(result == false);
    
    std::cout << "[PASS] test_rdata_invalid_path" << std::endl;
}

void test_rdata_empty_headers() {
    std::string filename = getTempDir() + "test_no_headers.RData";
    ReadingList readings;
    Reading r1;
    r1["x"] = "1";
    readings.push_back(r1);
    
    std::vector<std::string> headers;  // Empty headers
    
    bool result = RDataWriter::writeRData(filename, readings, headers);
    assert(result == true);
    
    std::remove(filename.c_str());
    std::cout << "[PASS] test_rdata_empty_headers" << std::endl;
}

void test_rdata_unicode_values() {
    std::string filename = getTempDir() + "test_unicode.RData";
    ReadingList readings;
    Reading r1;
    r1["location"] = "Café";  // Latin extended
    r1["temp"] = "20°C";       // Degree symbol
    readings.push_back(r1);
    
    std::vector<std::string> headers = {"location", "temp"};
    
    bool result = RDataWriter::writeRData(filename, readings, headers);
    assert(result == true);
    
    std::remove(filename.c_str());
    std::cout << "[PASS] test_rdata_unicode_values" << std::endl;
}

int main() {
    std::cout << "Running RData Writer Tests..." << std::endl;
    
    test_rdata_empty_data();
    test_rds_empty_data();
    test_rdata_single_row();
    test_rdata_multiple_rows();
    test_rdata_missing_columns();
    test_rdata_special_characters();
    test_rds_single_row();
    test_rdata_custom_table_name();
    test_rdata_invalid_path();
    test_rdata_empty_headers();
    test_rdata_unicode_values();
    
    std::cout << "\nAll RData Writer tests passed!" << std::endl;
    return 0;
}
