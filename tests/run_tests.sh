#!/bin/bash
# Test runner script for local development

set -e  # Exit on error

echo "================================"
echo "Building sensor-tools test suite"
echo "================================"

# Clean previous builds
rm -f *.o test_* sensor-data

# Build library objects
echo "Building library..."
g++ -c -std=c++11 -Iinclude src/csv_parser.cpp -o csv_parser.o
g++ -c -std=c++11 -Iinclude src/json_parser.cpp -o json_parser.o
g++ -c -std=c++11 -Iinclude src/error_detector.cpp -o error_detector.o
g++ -c -std=c++11 -Iinclude src/file_utils.cpp -o file_utils.o

echo ""
echo "================================"
echo "Running Unit Tests"
echo "================================"

# Run CSV Parser Tests
echo ""
echo "Testing CSV Parser..."
g++ -std=c++11 -Iinclude tests/test_csv_parser.cpp csv_parser.o -o test_csv_parser
./test_csv_parser

# Run JSON Parser Tests
echo ""
echo "Testing JSON Parser..."
g++ -std=c++11 -Iinclude tests/test_json_parser.cpp json_parser.o -o test_json_parser
./test_json_parser

# Run Error Detector Tests
echo ""
echo "Testing Error Detector..."
g++ -std=c++11 -Iinclude tests/test_error_detector.cpp error_detector.o -o test_error_detector
./test_error_detector

# Run File Utils Tests
echo ""
echo "Testing File Utils..."
g++ -std=c++11 -Iinclude tests/test_file_utils.cpp file_utils.o -o test_file_utils
./test_file_utils

echo ""
echo "================================"
echo "Building Main Application"
echo "================================"
g++ -std=c++11 -pthread -Iinclude src/sensor-data.cpp csv_parser.o json_parser.o error_detector.o file_utils.o -o sensor-data

echo ""
echo "================================"
echo "Running Integration Tests"
echo "================================"

# Create test data
echo '{"timestamp": "2026-01-17T10:00:00", "sensor_id": "sensor001", "type": "ds18b20", "value": "85"}' > test.out
echo '{"timestamp": "2026-01-17T10:01:00", "sensor_id": "sensor002", "type": "ds18b20", "value": "22.5"}' >> test.out

echo ""
echo "Testing list-errors command..."
./sensor-data list-errors test.out | grep -q "sensor001" && echo "[PASS] list-errors found error"

echo ""
echo "Testing transform command..."
./sensor-data transform -o output.csv test.out
grep -q "sensor001" output.csv && echo "[PASS] transform created CSV"
grep -q "sensor002" output.csv && echo "[PASS] transform included valid reading"

echo ""
echo "Testing transform with --remove-errors..."
./sensor-data transform --remove-errors -o output-clean.csv test.out
grep -q "sensor002" output-clean.csv && echo "[PASS] transform kept valid reading"
! grep -q "85" output-clean.csv && echo "[PASS] transform removed error reading"

# Cleanup
rm -f test.out output.csv output-clean.csv

echo ""
echo "================================"
echo "Running stdin/stdout Tests"
echo "================================"
bash tests/test_stdin_stdout.sh

echo ""
echo "================================"
echo "Running list-errors stdin Tests"
echo "================================"
bash tests/test_list_errors_stdin.sh

echo ""
echo "================================"
echo "Running summarise-errors Tests"
echo "================================"
bash tests/test_summarise_errors.sh

echo ""
echo "================================"
echo "Running stats Tests"
echo "================================"
bash tests/test_stats.sh

echo ""
echo "================================"
echo "Running CSV Input Tests"
echo "================================"
bash tests/test_csv_input.sh

echo ""
echo "================================"
echo "Running Error Handling Tests"
echo "================================"
bash tests/test_error_handling.sh

echo ""
echo "================================"
echo "All tests passed!"
echo "================================"
