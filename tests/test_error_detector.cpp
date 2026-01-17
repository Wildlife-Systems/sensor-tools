#include "../include/error_detector.h"
#include <cassert>
#include <iostream>

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

int main() {
    std::cout << "Running Error Detector Tests..." << std::endl;
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
    std::cout << "All Error Detector tests passed!" << std::endl;
    return 0;
}
