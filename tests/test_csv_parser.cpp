#include "../include/csv_parser.h"
#include <cassert>
#include <iostream>
#include <sstream>

void test_simple_csv() {
    std::string line = "field1,field2,field3";
    auto result = CsvParser::parseCsvLine(line);
    assert(result.size() == 3);
    assert(result[0] == "field1");
    assert(result[1] == "field2");
    assert(result[2] == "field3");
    std::cout << "[PASS] test_simple_csv" << std::endl;
}

void test_csv_with_quotes() {
    std::string line = "\"field1\",\"field,2\",\"field3\"";
    auto result = CsvParser::parseCsvLine(line);
    assert(result.size() == 3);
    assert(result[0] == "field1");
    assert(result[1] == "field,2");
    assert(result[2] == "field3");
    std::cout << "[PASS] test_csv_with_quotes" << std::endl;
}

void test_csv_with_escaped_quotes() {
    std::string line = "\"field1\",\"field\"\"2\"\"\",\"field3\"";
    auto result = CsvParser::parseCsvLine(line);
    assert(result.size() == 3);
    assert(result[0] == "field1");
    assert(result[1] == "field\"2\"");
    assert(result[2] == "field3");
    std::cout << "[PASS] test_csv_with_escaped_quotes" << std::endl;
}

void test_csv_empty_fields() {
    std::string line = "field1,,field3";
    auto result = CsvParser::parseCsvLine(line);
    assert(result.size() == 3);
    assert(result[0] == "field1");
    assert(result[1] == "");
    assert(result[2] == "field3");
    std::cout << "[PASS] test_csv_empty_fields" << std::endl;
}

void test_csv_multiline() {
    std::stringstream input("\"field1\",\"field\nwith\nnewlines\",\"field3\"");
    std::string line;
    std::getline(input, line);
    bool needMore;
    auto result = CsvParser::parseCsvLine(input, line, needMore);
    assert(result.size() == 3);
    assert(result[0] == "field1");
    assert(result[1] == "field\nwith\nnewlines");
    assert(result[2] == "field3");
    std::cout << "[PASS] test_csv_multiline" << std::endl;
}

void test_csv_crlf_line_ending() {
    std::string line = "field1,field2,field3\r";  // Windows line ending remnant
    auto result = CsvParser::parseCsvLine(line);
    assert(result.size() == 3);
    // Should handle trailing \r
    std::cout << "[PASS] test_csv_crlf_line_ending" << std::endl;
}

void test_csv_single_field() {
    std::string line = "single";
    auto result = CsvParser::parseCsvLine(line);
    assert(result.size() == 1);
    assert(result[0] == "single");
    std::cout << "[PASS] test_csv_single_field" << std::endl;
}

void test_csv_empty_line() {
    std::string line = "";
    auto result = CsvParser::parseCsvLine(line);
    // Empty line should produce minimal output
    std::cout << "[PASS] test_csv_empty_line" << std::endl;
}

void test_csv_trailing_comma() {
    std::string line = "field1,field2,";
    auto result = CsvParser::parseCsvLine(line);
    assert(result.size() == 3);
    assert(result[0] == "field1");
    assert(result[1] == "field2");
    assert(result[2] == "");
    std::cout << "[PASS] test_csv_trailing_comma" << std::endl;
}

void test_csv_leading_comma() {
    std::string line = ",field2,field3";
    auto result = CsvParser::parseCsvLine(line);
    assert(result.size() == 3);
    assert(result[0] == "");
    assert(result[1] == "field2");
    assert(result[2] == "field3");
    std::cout << "[PASS] test_csv_leading_comma" << std::endl;
}

void test_csv_spaces_in_field() {
    std::string line = "field 1,field 2,field 3";
    auto result = CsvParser::parseCsvLine(line);
    assert(result.size() == 3);
    assert(result[0] == "field 1");
    assert(result[1] == "field 2");
    assert(result[2] == "field 3");
    std::cout << "[PASS] test_csv_spaces_in_field" << std::endl;
}

void test_csv_quoted_empty() {
    std::string line = "\"\",\"field2\",\"\"";
    auto result = CsvParser::parseCsvLine(line);
    assert(result.size() == 3);
    assert(result[0] == "");
    assert(result[1] == "field2");
    assert(result[2] == "");
    std::cout << "[PASS] test_csv_quoted_empty" << std::endl;
}

void test_csv_unicode_content() {
    std::string line = "sensor_id,name,value";
    // This tests that basic ASCII works, unicode in UTF-8 should pass through
    std::string line2 = "s1,\"Température\",22.5";
    auto result = CsvParser::parseCsvLine(line2);
    assert(result.size() == 3);
    assert(result[1] == "Température");
    std::cout << "[PASS] test_csv_unicode_content" << std::endl;
}

void test_csv_many_fields() {
    std::string line = "a,b,c,d,e,f,g,h,i,j";
    auto result = CsvParser::parseCsvLine(line);
    assert(result.size() == 10);
    assert(result[0] == "a");
    assert(result[9] == "j");
    std::cout << "[PASS] test_csv_many_fields" << std::endl;
}

int main() {
    std::cout << "Running CSV Parser Tests..." << std::endl;
    test_simple_csv();
    test_csv_with_quotes();
    test_csv_with_escaped_quotes();
    test_csv_empty_fields();
    test_csv_multiline();
    test_csv_crlf_line_ending();
    test_csv_single_field();
    test_csv_empty_line();
    test_csv_trailing_comma();
    test_csv_leading_comma();
    test_csv_spaces_in_field();
    test_csv_quoted_empty();
    test_csv_unicode_content();
    test_csv_many_fields();
    std::cout << "All CSV Parser tests passed!" << std::endl;
    return 0;
}
