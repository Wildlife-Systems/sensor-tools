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

void test_json_with_array_value() {
    // Test object with an array as a value
    std::string line = R"({"sensor": "ds18b20", "tags": ["indoor", "floor1", "room3"], "value": 22.5})";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 1);
    assert(result[0]["sensor"] == "ds18b20");
    assert(result[0]["tags"] == R"("indoor", "floor1", "room3")");
    assert(result[0]["value"] == "22.5");
    std::cout << "[PASS] test_json_with_array_value" << std::endl;
}

void test_json_with_nested_array() {
    // Test array value with nested arrays
    std::string line = R"({"data": [[1, 2], [3, 4]], "name": "matrix"})";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 1);
    assert(result[0]["data"] == "[1, 2], [3, 4]");
    assert(result[0]["name"] == "matrix");
    std::cout << "[PASS] test_json_with_nested_array" << std::endl;
}

void test_json_with_array_containing_strings_with_brackets() {
    // Test array with strings containing brackets (must respect string escaping)
    std::string line = R"({"items": ["[test]", "a]b", "c[d"], "count": 3})";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 1);
    assert(result[0]["items"] == R"("[test]", "a]b", "c[d")");
    assert(result[0]["count"] == "3");
    std::cout << "[PASS] test_json_with_array_containing_strings_with_brackets" << std::endl;
}

void test_json_with_nested_object_value() {
    // Test object with a nested object as a value
    std::string line = R"({"sensor": "ds18b20", "metadata": {"location": "room1", "floor": 2}, "value": 22.5})";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 1);
    assert(result[0]["sensor"] == "ds18b20");
    assert(result[0]["metadata"] == R"({"location": "room1", "floor": 2})");
    assert(result[0]["value"] == "22.5");
    std::cout << "[PASS] test_json_with_nested_object_value" << std::endl;
}

void test_json_with_deeply_nested_object() {
    // Test deeply nested object (multiple levels)
    std::string line = R"({"outer": {"inner": {"deep": "value"}}, "name": "test"})";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 1);
    assert(result[0]["outer"] == R"({"inner": {"deep": "value"}})");
    assert(result[0]["name"] == "test");
    std::cout << "[PASS] test_json_with_deeply_nested_object" << std::endl;
}

void test_json_with_nested_object_containing_strings_with_braces() {
    // Test nested object with strings containing braces (must respect string escaping)
    std::string line = R"({"info": {"desc": "a {b} c", "note": "x}y"}, "id": 1})";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 1);
    assert(result[0]["info"] == R"({"desc": "a {b} c", "note": "x}y"})");
    assert(result[0]["id"] == "1");
    std::cout << "[PASS] test_json_with_nested_object_containing_strings_with_braces" << std::endl;
}

void test_json_with_escaped_quotes_in_nested_structures() {
    // Test escaped quotes inside array strings
    std::string line = R"({"labels": ["say \"hello\"", "test"], "ok": true})";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 1);
    assert(result[0]["labels"] == R"("say \"hello\"", "test")");
    assert(result[0]["ok"] == "true");
    std::cout << "[PASS] test_json_with_escaped_quotes_in_nested_structures" << std::endl;
}

void test_json_boolean_values() {
    std::string line = R"({"active": true, "disabled": false, "name": "test"})";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 1);
    assert(result[0]["active"] == "true");
    assert(result[0]["disabled"] == "false");
    assert(result[0]["name"] == "test");
    std::cout << "[PASS] test_json_boolean_values" << std::endl;
}

void test_json_null_value() {
    std::string line = R"({"sensor_id": "s1", "value": null, "name": "test"})";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 1);
    assert(result[0]["sensor_id"] == "s1");
    assert(result[0]["value"] == "null");
    std::cout << "[PASS] test_json_null_value" << std::endl;
}

void test_json_negative_numbers() {
    std::string line = R"({"temperature": -127, "offset": -0.5})";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 1);
    assert(result[0]["temperature"] == "-127");
    assert(result[0]["offset"] == "-0.5");
    std::cout << "[PASS] test_json_negative_numbers" << std::endl;
}

void test_json_scientific_notation() {
    std::string line = R"({"large": 1.5e10, "small": 2.5e-5})";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 1);
    // Scientific notation should be parsed
    std::cout << "[PASS] test_json_scientific_notation" << std::endl;
}

void test_json_empty_object() {
    std::string line = R"({})";
    auto result = JsonParser::parseJsonLine(line);
    // Empty object produces no readings (nothing to record)
    assert(result.size() == 0);
    std::cout << "[PASS] test_json_empty_object" << std::endl;
}

void test_json_empty_array() {
    std::string line = R"([])";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 0);
    std::cout << "[PASS] test_json_empty_array" << std::endl;
}

void test_json_whitespace_variations() {
    std::string line = R"(  {  "key1"  :  "value1"  ,  "key2"  :  "value2"  }  )";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 1);
    assert(result[0]["key1"] == "value1");
    assert(result[0]["key2"] == "value2");
    std::cout << "[PASS] test_json_whitespace_variations" << std::endl;
}

void test_json_integer_values() {
    std::string line = R"({"count": 42, "zero": 0, "big": 999999})";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 1);
    assert(result[0]["count"] == "42");
    assert(result[0]["zero"] == "0");
    assert(result[0]["big"] == "999999");
    std::cout << "[PASS] test_json_integer_values" << std::endl;
}

void test_json_unicode_string() {
    std::string line = R"({"name": "Température", "city": "北京"})";
    auto result = JsonParser::parseJsonLine(line);
    assert(result.size() == 1);
    assert(result[0]["name"] == "Température");
    std::cout << "[PASS] test_json_unicode_string" << std::endl;
}

int main() {
    std::cout << "Running JSON Parser Tests..." << std::endl;
    test_simple_json();
    test_json_with_numbers();
    test_json_sensor_data();
    test_json_array_two_objects();
    test_json_array_multiple_fields();
    test_json_array_single_object();
    test_json_with_array_value();
    test_json_with_nested_array();
    test_json_with_array_containing_strings_with_brackets();
    test_json_with_nested_object_value();
    test_json_with_deeply_nested_object();
    test_json_with_nested_object_containing_strings_with_braces();
    test_json_with_escaped_quotes_in_nested_structures();
    test_json_boolean_values();
    test_json_null_value();
    test_json_negative_numbers();
    test_json_scientific_notation();
    test_json_empty_object();
    test_json_empty_array();
    test_json_whitespace_variations();
    test_json_integer_values();
    test_json_unicode_string();
    std::cout << "All JSON Parser tests passed!" << std::endl;
    return 0;
}
