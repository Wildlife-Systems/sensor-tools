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

int main() {
    std::cout << "Running CSV Parser Tests..." << std::endl;
    test_simple_csv();
    test_csv_with_quotes();
    test_csv_with_escaped_quotes();
    test_csv_empty_fields();
    test_csv_multiline();
    std::cout << "All CSV Parser tests passed!" << std::endl;
    return 0;
}
