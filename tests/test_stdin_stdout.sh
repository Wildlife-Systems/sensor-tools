#!/bin/bash
# Integration tests for stdin/stdout functionality

set -e  # Exit on error

echo "================================"
echo "Testing stdin/stdout functionality"
echo "================================"

# Build sensor-data if not already built
if [ ! -f sensor-data ]; then
    echo "Building sensor-data..."
    g++ -c -std=c++11 -pthread -Iinclude src/csv_parser.cpp -o csv_parser.o
    g++ -c -std=c++11 -pthread -Iinclude src/json_parser.cpp -o json_parser.o
    g++ -c -std=c++11 -pthread -Iinclude src/error_detector.cpp -o error_detector.o
    g++ -c -std=c++11 -pthread -Iinclude src/file_utils.cpp -o file_utils.o
    g++ -std=c++11 -pthread -Iinclude src/sensor-data.cpp csv_parser.o json_parser.o error_detector.o file_utils.o -o sensor-data
fi

PASSED=0
FAILED=0

# Helper function to run a test
run_test() {
    local test_name="$1"
    local test_command="$2"
    local expected_output="$3"
    
    echo ""
    echo "Test: $test_name"
    
    if result=$(eval "$test_command" 2>&1); then
        if echo "$result" | grep -q "$expected_output"; then
            echo "  ✓ PASS"
            PASSED=$((PASSED + 1))
        else
            echo "  ✗ FAIL - Expected output not found"
            echo "  Expected: $expected_output"
            echo "  Got: $result"
            FAILED=$((FAILED + 1))
        fi
    else
        echo "  ✗ FAIL - Command failed"
        echo "  Output: $result"
        FAILED=$((FAILED + 1))
    fi
}

# Create temporary test data
cat > /tmp/test_stdin_json.txt << 'EOF'
{"sensor":"ds18b20","value":"22.5","unit":"C","timestamp":"2024-01-01T00:00:00Z"}
{"sensor":"bme280","value":"1013.25","unit":"hPa","timestamp":"2024-01-01T00:00:01Z"}
{"sensor":"ds18b20","value":"23.0","unit":"C","timestamp":"2024-01-01T00:00:02Z"}
EOF

cat > /tmp/test_stdin_csv.txt << 'EOF'
sensor,value,unit,timestamp
ds18b20,22.5,C,2024-01-01T00:00:00Z
bme280,1013.25,hPa,2024-01-01T00:00:01Z
ds18b20,23.0,C,2024-01-01T00:00:02Z
EOF

cat > /tmp/test_stdin_json_error.txt << 'EOF'
{"sensor":"ds18b20","value":"22.5","unit":"C","timestamp":"2024-01-01T00:00:00Z"}
{"sensor":"ds18b20","value":"85","unit":"C","timestamp":"2024-01-01T00:00:01Z"}
{"sensor":"ds18b20","value":"23.0","unit":"C","timestamp":"2024-01-01T00:00:02Z"}
EOF

# Test 1: JSON stdin to stdout (CSV output)
run_test "JSON stdin to stdout" \
    "cat /tmp/test_stdin_json.txt | ./sensor-data transform -of csv" \
    "sensor,timestamp,unit,value"

# Test 2: JSON stdin with output file (CSV output)
run_test "JSON stdin to output file" \
    "cat /tmp/test_stdin_json.txt | ./sensor-data transform -of csv -o /tmp/test_output.csv && head -1 /tmp/test_output.csv" \
    "sensor,timestamp,unit,value"

# Test 3: CSV stdin with format flag (CSV output)
run_test "CSV stdin with -if csv" \
    "cat /tmp/test_stdin_csv.txt | ./sensor-data transform -if csv -of csv" \
    "sensor,timestamp,unit,value"

# Test 4: JSON stdin with redirect (JSON output - check for content)
run_test "JSON stdin with redirect" \
    "./sensor-data transform < /tmp/test_stdin_json.txt" \
    "ds18b20"

# Test 5: CSV stdin with redirect and format flag (CSV output)
run_test "CSV stdin with redirect and -if csv" \
    "./sensor-data transform -if csv -of csv < /tmp/test_stdin_csv.txt" \
    "bme280"

# Test 6: JSON stdin with --remove-errors flag
run_test "JSON stdin with --remove-errors" \
    "cat /tmp/test_stdin_json_error.txt | ./sensor-data transform -of csv --remove-errors | wc -l" \
    "3"

# Test 7: JSON stdin to file with --remove-errors
run_test "JSON stdin to file with --remove-errors" \
    "cat /tmp/test_stdin_json_error.txt | ./sensor-data transform -of csv --remove-errors -o /tmp/test_error_output.csv && wc -l < /tmp/test_error_output.csv" \
    "3"

# Test 8: Verify CSV output has correct number of columns
run_test "CSV output has correct columns" \
    "cat /tmp/test_stdin_json.txt | ./sensor-data transform -of csv | head -1 | awk -F',' '{print NF}'" \
    "4"

# Test 9: JSON stdin with --only-value filter
run_test "JSON stdin with --only-value filter" \
    "cat /tmp/test_stdin_json.txt | ./sensor-data transform -of csv --only-value sensor:ds18b20 | tail -n +2 | wc -l" \
    "2"

# Test 10: JSON stdin with --not-empty filter
run_test "JSON stdin with --not-empty filter" \
    "cat /tmp/test_stdin_json.txt | ./sensor-data transform -of csv --not-empty value | tail -n +2 | wc -l" \
    "3"

# Test 11: Empty stdin handling
run_test "Empty stdin handling" \
    "echo '' | ./sensor-data transform 2>&1" \
    "Error: No input data"

# Test 12: Pipe chain with output file
run_test "Pipe chain with output file" \
    "cat /tmp/test_stdin_json.txt | ./sensor-data transform -of csv -o /tmp/test_pipe.csv && cat /tmp/test_pipe.csv | wc -l" \
    "4"

# Cleanup
rm -f /tmp/test_stdin_json.txt /tmp/test_stdin_csv.txt /tmp/test_stdin_json_error.txt
rm -f /tmp/test_output.csv /tmp/test_error_output.csv /tmp/test_pipe.csv

echo ""
echo "================================"
echo "Test Results"
echo "================================"
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo ""

if [ $FAILED -eq 0 ]; then
    echo "All tests passed! ✓"
    exit 0
else
    echo "Some tests failed! ✗"
    exit 1
fi
