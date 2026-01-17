#include "../include/json_parser.h"
#include <cassert>
#include <iostream>

void test_simple_json() {
    std::string line = R"({"key1": "value1", "key2": "value2"})";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() >= 1);
    assert(result[0]["key1"] == "value1");
    assert(result[0]["key2"] == "value2");
    std::cout << "[PASS] test_simple_json" << std::endl;
}

void test_json_with_numbers() {
    std::string line = R"({"temperature": 22.5, "humidity": 45})";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() >= 1);
    assert(result[0]["temperature"] == "22.5");
    assert(result[0]["humidity"] == "45");
    std::cout << "[PASS] test_json_with_numbers" << std::endl;
}

void test_json_sensor_data() {
    std::string line = R"({"timestamp": "2026-01-17T10:00:00", "sensor_id": "sensor001", "type": "ds18b20", "value": "22.5"})";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() >= 1);
    assert(result[0]["timestamp"] == "2026-01-17T10:00:00");
    assert(result[0]["sensor_id"] == "sensor001");
    assert(result[0]["type"] == "ds18b20");
    assert(result[0]["value"] == "22.5");
    std::cout << "[PASS] test_json_sensor_data" << std::endl;
}

int main() {
    std::cout << "Running JSON Parser Tests..." << std::endl;
    test_simple_json();
    test_json_with_numbers();
    test_json_sensor_data();
    std::cout << "All JSON Parser tests passed!" << std::endl;
    return 0;
}
