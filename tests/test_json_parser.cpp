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

void test_json_array_two_objects() {
    std::string line = R"([ { "sensor": "ds18b20", "value": 85 }, { "sensor": "ds18b20", "value": 14.5 } ])";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 2);
    assert(result[0]["sensor"] == "ds18b20");
    assert(result[0]["value"] == "85");
    assert(result[1]["sensor"] == "ds18b20");
    assert(result[1]["value"] == "14.5");
    std::cout << "[PASS] test_json_array_two_objects" << std::endl;
}

void test_json_array_multiple_fields() {
    std::string line = R"([ { "sensor": "ds18b20", "measures": "temperature", "value": 14.625, "unit": "Celsius", "sensor_id": "28-00000fa3d75b" }, { "sensor": "onboard_gpu", "measures": "temperature", "value": 54, "unit": "Celsius" } ])";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 2);
    assert(result[0]["sensor"] == "ds18b20");
    assert(result[0]["measures"] == "temperature");
    assert(result[0]["value"] == "14.625");
    assert(result[0]["unit"] == "Celsius");
    assert(result[0]["sensor_id"] == "28-00000fa3d75b");
    assert(result[1]["sensor"] == "onboard_gpu");
    assert(result[1]["value"] == "54");
    std::cout << "[PASS] test_json_array_multiple_fields" << std::endl;
}

void test_json_array_single_object() {
    std::string line = R"([ { "sensor": "ds18b20", "value": 22.5 } ])";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 1);
    assert(result[0]["sensor"] == "ds18b20");
    assert(result[0]["value"] == "22.5");
    std::cout << "[PASS] test_json_array_single_object" << std::endl;
}

int main() {
    std::cout << "Running JSON Parser Tests..." << std::endl;
    test_simple_json();
    test_json_with_numbers();
    test_json_sensor_data();
    test_json_array_two_objects();
    test_json_array_multiple_fields();
    test_json_array_single_object();
    std::cout << "All JSON Parser tests passed!" << std::endl;
    return 0;
}
