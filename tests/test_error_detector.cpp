#include "../include/error_detector.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sys/stat.h>

// Helper to create a directory
bool createDir(const std::string& path) {
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
}

// Helper to remove a file
void removeFile(const std::string& path) {
    remove(path.c_str());
}

// Helper to remove a directory
void removeDir(const std::string& path) {
    rmdir(path.c_str());
}

void test_ds18b20_error_detection() {
    std::map<std::string, std::string> reading;
    reading["sensor"] = "ds18b20";
    reading["value"] = "85";
    reading["sensor_id"] = "sensor001";
    
    assert(ErrorDetector::isErrorReading(reading) == true);
    std::cout << "[PASS] test_ds18b20_error_detection" << std::endl;
}

void test_ds18b20_error_minus_127() {
    std::map<std::string, std::string> reading;
    reading["sensor"] = "ds18b20";
    reading["value"] = "-127";
    reading["sensor_id"] = "sensor001";
    
    assert(ErrorDetector::isErrorReading(reading) == true);
    std::cout << "[PASS] test_ds18b20_error_minus_127" << std::endl;
}

void test_ds18b20_error_minus_127_temperature() {
    std::map<std::string, std::string> reading;
    reading["sensor"] = "ds18b20";
    reading["temperature"] = "-127";
    reading["sensor_id"] = "sensor001";
    
    assert(ErrorDetector::isErrorReading(reading) == true);
    std::cout << "[PASS] test_ds18b20_error_minus_127_temperature" << std::endl;
}

void test_error_description_85() {
    std::map<std::string, std::string> reading;
    reading["sensor"] = "ds18b20";
    reading["value"] = "85";
    reading["sensor_id"] = "sensor001";
    
    std::string desc = ErrorDetector::getErrorDescription(reading);
    assert(desc == "DS18B20 communication error (value=85)");
    std::cout << "[PASS] test_error_description_85" << std::endl;
}

void test_error_description_minus_127() {
    std::map<std::string, std::string> reading;
    reading["sensor"] = "ds18b20";
    reading["value"] = "-127";
    reading["sensor_id"] = "sensor001";
    
    std::string desc = ErrorDetector::getErrorDescription(reading);
    assert(desc == "DS18B20 disconnected or power-on reset (value=-127)");
    std::cout << "[PASS] test_error_description_minus_127" << std::endl;
}

void test_error_description_no_error() {
    std::map<std::string, std::string> reading;
    reading["sensor"] = "ds18b20";
    reading["value"] = "22.5";
    reading["sensor_id"] = "sensor001";
    
    std::string desc = ErrorDetector::getErrorDescription(reading);
    assert(desc == "");
    std::cout << "[PASS] test_error_description_no_error" << std::endl;
}

void test_ds18b20_valid_reading() {
    std::map<std::string, std::string> reading;
    reading["sensor"] = "ds18b20";
    reading["value"] = "22.5";
    reading["sensor_id"] = "sensor001";
    
    assert(ErrorDetector::isErrorReading(reading) == false);
    std::cout << "[PASS] test_ds18b20_valid_reading" << std::endl;
}

void test_ds18b20_case_insensitive() {
    std::map<std::string, std::string> reading;
    reading["sensor"] = "DS18B20";
    reading["value"] = "85";
    reading["sensor_id"] = "sensor001";
    
    assert(ErrorDetector::isErrorReading(reading) == true);
    std::cout << "[PASS] test_ds18b20_case_insensitive" << std::endl;
}

void test_temperature_field() {
    std::map<std::string, std::string> reading;
    reading["sensor"] = "ds18b20";
    reading["temperature"] = "85";
    reading["sensor_id"] = "sensor001";
    
    assert(ErrorDetector::isErrorReading(reading) == true);
    std::cout << "[PASS] test_temperature_field" << std::endl;
}

void test_non_ds18b20_sensor() {
    std::map<std::string, std::string> reading;
    reading["sensor"] = "dht22";
    reading["value"] = "85";
    reading["sensor_id"] = "sensor001";
    
    assert(ErrorDetector::isErrorReading(reading) == false);
    std::cout << "[PASS] test_non_ds18b20_sensor" << std::endl;
}

// Tests for custom error file loading
void test_custom_error_file() {
    // Create a temporary directory with custom error file
    std::string testDir = "test_sensor_errors";
    createDir(testDir);
    
    // Create a custom sensor error file for a dummy sensor
    std::ofstream errorFile(testDir + "/dummy_sensor.errors");
    errorFile << "# Test sensor errors\n";
    errorFile << "reading:999:Dummy sensor overflow error\n";
    errorFile << "reading:-999:Dummy sensor underflow error\n";
    errorFile.close();
    
    // Load from custom directory
    ErrorDetector::loadErrorDefinitions(testDir);
    
    // Test that custom error is detected
    std::map<std::string, std::string> reading;
    reading["sensor"] = "dummy_sensor";
    reading["reading"] = "999";
    
    assert(ErrorDetector::isErrorReading(reading) == true);
    std::string desc = ErrorDetector::getErrorDescription(reading);
    assert(desc == "Dummy sensor overflow error");
    
    // Test underflow error
    reading["reading"] = "-999";
    assert(ErrorDetector::isErrorReading(reading) == true);
    desc = ErrorDetector::getErrorDescription(reading);
    assert(desc == "Dummy sensor underflow error");
    
    // Test valid reading (no error)
    reading["reading"] = "42";
    assert(ErrorDetector::isErrorReading(reading) == false);
    
    // Cleanup
    removeFile(testDir + "/dummy_sensor.errors");
    removeDir(testDir);
    
    // Reset to builtin definitions for other tests
    ErrorDetector::loadErrorDefinitions("/nonexistent/path");
    
    std::cout << "[PASS] test_custom_error_file" << std::endl;
}

void test_multiple_sensor_error_files() {
    // Create a temporary directory with multiple sensor error files
    std::string testDir = "test_multi_errors";
    createDir(testDir);
    
    // Create error file for sensor_a
    std::ofstream fileA(testDir + "/sensor_a.errors");
    fileA << "value:100:Sensor A high error\n";
    fileA.close();
    
    // Create error file for sensor_b
    std::ofstream fileB(testDir + "/sensor_b.errors");
    fileB << "temp:-50:Sensor B low temp error\n";
    fileB.close();
    
    // Load from custom directory
    ErrorDetector::loadErrorDefinitions(testDir);
    
    // Test sensor_a error
    std::map<std::string, std::string> readingA;
    readingA["sensor"] = "sensor_a";
    readingA["value"] = "100";
    assert(ErrorDetector::isErrorReading(readingA) == true);
    assert(ErrorDetector::getErrorDescription(readingA) == "Sensor A high error");
    
    // Test sensor_b error
    std::map<std::string, std::string> readingB;
    readingB["sensor"] = "sensor_b";
    readingB["temp"] = "-50";
    assert(ErrorDetector::isErrorReading(readingB) == true);
    assert(ErrorDetector::getErrorDescription(readingB) == "Sensor B low temp error");
    
    // Test that sensor_a doesn't match sensor_b's error
    readingA["temp"] = "-50";
    readingA.erase("value");
    assert(ErrorDetector::isErrorReading(readingA) == false);
    
    // Cleanup
    removeFile(testDir + "/sensor_a.errors");
    removeFile(testDir + "/sensor_b.errors");
    removeDir(testDir);
    
    // Reset to builtin definitions
    ErrorDetector::loadErrorDefinitions("/nonexistent/path");
    
    std::cout << "[PASS] test_multiple_sensor_error_files" << std::endl;
}

void test_fallback_to_builtin() {
    // Load from non-existent directory - should fall back to builtin
    ErrorDetector::loadErrorDefinitions("/this/path/does/not/exist");
    
    // Should still detect DS18B20 errors via builtin definitions
    std::map<std::string, std::string> reading;
    reading["sensor"] = "ds18b20";
    reading["value"] = "85";
    
    assert(ErrorDetector::isErrorReading(reading) == true);
    assert(ErrorDetector::getErrorDescription(reading) == "DS18B20 communication error (value=85)");
    
    std::cout << "[PASS] test_fallback_to_builtin" << std::endl;
}

void test_case_insensitive_custom_sensor() {
    std::string testDir = "test_case_errors";
    createDir(testDir);
    
    // Create error file with lowercase sensor name
    std::ofstream errorFile(testDir + "/testsensor.errors");
    errorFile << "code:ERR01:Test error code\n";
    errorFile.close();
    
    ErrorDetector::loadErrorDefinitions(testDir);
    
    // Test with various case combinations
    std::map<std::string, std::string> reading;
    reading["code"] = "ERR01";
    
    reading["sensor"] = "TestSensor";
    assert(ErrorDetector::isErrorReading(reading) == true);
    
    reading["sensor"] = "TESTSENSOR";
    assert(ErrorDetector::isErrorReading(reading) == true);
    
    reading["sensor"] = "testsensor";
    assert(ErrorDetector::isErrorReading(reading) == true);
    
    // Cleanup
    removeFile(testDir + "/testsensor.errors");
    removeDir(testDir);
    ErrorDetector::loadErrorDefinitions("/nonexistent/path");
    
    std::cout << "[PASS] test_case_insensitive_custom_sensor" << std::endl;
}

int main() {
    std::cout << "Running Error Detector Tests..." << std::endl;
    
    // Reset to ensure clean state with builtin definitions
    ErrorDetector::loadErrorDefinitions("/nonexistent/path");
    
    test_ds18b20_error_detection();
    test_ds18b20_error_minus_127();
    test_ds18b20_error_minus_127_temperature();
    test_error_description_85();
    test_error_description_minus_127();
    test_error_description_no_error();
    test_ds18b20_valid_reading();
    test_ds18b20_case_insensitive();
    test_temperature_field();
    test_non_ds18b20_sensor();
    
    // Custom error file tests
    test_custom_error_file();
    test_multiple_sensor_error_files();
    test_fallback_to_builtin();
    test_case_insensitive_custom_sensor();
    
    std::cout << "All Error Detector tests passed!" << std::endl;
    return 0;
}
