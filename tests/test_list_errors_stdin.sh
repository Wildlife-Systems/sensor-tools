#!/bin/bash
# Integration tests for list-errors stdin functionality

set -e  # Exit on error

echo "================================"
echo "Testing list-errors stdin functionality"
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

# Test 1: JSON stdin with error value 85
echo "Test 1: JSON stdin with error value 85"
result=$(echo '{"sensor":"ds18b20","value":"85","sensor_id":"001"}' | ./sensor-data list-errors)
if echo "$result" | grep -q "stdin:1" && echo "$result" | grep -q "communication error"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 2: JSON stdin with error value -127
echo ""
echo "Test 2: JSON stdin with error value -127"
result=$(echo '{"sensor":"ds18b20","value":"-127","sensor_id":"002"}' | ./sensor-data list-errors)
if echo "$result" | grep -q "stdin:1" && echo "$result" | grep -q "disconnected or power-on reset"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 3: JSON stdin with valid reading (should return nothing)
echo ""
echo "Test 3: JSON stdin with valid reading"
result=$(echo '{"sensor":"ds18b20","value":"22.5","sensor_id":"003"}' | ./sensor-data list-errors)
if [ -z "$result" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected no output"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 4: Multiple lines with errors
echo ""
echo "Test 4: Multiple lines with mixed errors"
result=$(cat <<'EOF' | ./sensor-data list-errors
{"sensor":"ds18b20","value":"22.5","sensor_id":"001"}
{"sensor":"ds18b20","value":"85","sensor_id":"002"}
{"sensor":"ds18b20","value":"-127","sensor_id":"003"}
{"sensor":"ds18b20","value":"23.0","sensor_id":"004"}
EOF
)
if echo "$result" | grep -q "stdin:2.*communication error" && echo "$result" | grep -q "stdin:3.*disconnected"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 5: CSV stdin with error value 85
echo ""
echo "Test 5: CSV stdin with -f csv flag and error value 85"
result=$(cat <<'EOF' | ./sensor-data list-errors -f csv
sensor,value,sensor_id
ds18b20,85,001
EOF
)
if echo "$result" | grep -q "stdin:2" && echo "$result" | grep -q "communication error"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 6: CSV stdin with error value -127
echo ""
echo "Test 6: CSV stdin with -f csv flag and error value -127"
result=$(cat <<'EOF' | ./sensor-data list-errors -f csv
sensor,value,sensor_id
ds18b20,-127,002
EOF
)
if echo "$result" | grep -q "stdin:2" && echo "$result" | grep -q "disconnected or power-on reset"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 7: Temperature field instead of value field
echo ""
echo "Test 7: JSON stdin with temperature field"
result=$(echo '{"sensor":"ds18b20","temperature":"85","sensor_id":"004"}' | ./sensor-data list-errors)
if echo "$result" | grep -q "stdin:1" && echo "$result" | grep -q "communication error.*temperature=85"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 8: Non-DS18B20 sensor with value 85 (should not be detected)
echo ""
echo "Test 8: Non-DS18B20 sensor with value 85"
result=$(echo '{"sensor":"dht22","value":"85","sensor_id":"005"}' | ./sensor-data list-errors)
if [ -z "$result" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected no output"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 9: Piped input
echo ""
echo "Test 9: Piped input from cat"
echo '{"sensor":"ds18b20","value":"85","sensor_id":"006"}' > /tmp/test_error_input.txt
result=$(cat /tmp/test_error_input.txt | ./sensor-data list-errors)
if echo "$result" | grep -q "stdin:1" && echo "$result" | grep -q "communication error"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi
rm -f /tmp/test_error_input.txt

# Test 10: Redirected input
echo ""
echo "Test 10: Redirected input"
echo '{"sensor":"ds18b20","value":"-127","sensor_id":"007"}' > /tmp/test_error_redirect.txt
result=$(./sensor-data list-errors < /tmp/test_error_redirect.txt)
if echo "$result" | grep -q "stdin:1" && echo "$result" | grep -q "disconnected or power-on reset"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi
rm -f /tmp/test_error_redirect.txt

# Summary
echo ""
echo "================================"
echo "Test Summary"
echo "================================"
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ $FAILED -eq 0 ]; then
    echo "✓ All list-errors stdin tests passed!"
    exit 0
else
    echo "✗ Some tests failed"
    exit 1
fi
