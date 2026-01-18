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

# Test: list-errors --help shows usage
echo "Test: list-errors --help shows usage"
result=$(./sensor-data list-errors --help 2>&1) || true
if echo "$result" | grep -q -i "list-errors\|error\|usage"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected list-errors help text"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

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
echo "Test 5: CSV stdin with -if csv flag and error value 85"
result=$(cat <<'EOF' | ./sensor-data list-errors -if csv
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
echo "Test 6: CSV stdin with -if csv flag and error value -127"
result=$(cat <<'EOF' | ./sensor-data list-errors -if csv
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

# Test 11: Unknown option should fail
echo ""
echo "Test 11: Unknown option (--unknown)"
result=$(echo '{"sensor":"ds18b20","value":"85"}' | ./sensor-data list-errors --unknown 2>&1) && exit_code=$? || exit_code=$?
if [ $exit_code -ne 0 ] && echo "$result" | grep -q "Unknown option"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected error for unknown option"
    echo "  Exit code: $exit_code"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 12: Unknown short option should fail
echo ""
echo "Test 12: Unknown short option (-x)"
result=$(echo '{"sensor":"ds18b20","value":"85"}' | ./sensor-data list-errors -x 2>&1) && exit_code=$? || exit_code=$?
if [ $exit_code -ne 0 ] && echo "$result" | grep -q "Unknown option"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected error for unknown option"
    echo "  Exit code: $exit_code"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 13: Known options should work (-v verbose)
echo ""
echo "Test 13: Known option -v (verbose)"
result=$(echo '{"sensor":"ds18b20","value":"85"}' | ./sensor-data list-errors -v 2>&1)
if echo "$result" | grep -q "communication error"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 14: Known option with argument (-if csv)
echo ""
echo "Test 14: Known option with argument (-if csv)"
result=$(cat <<'EOF' | ./sensor-data list-errors -if csv 2>&1
sensor,value,sensor_id
ds18b20,85,001
EOF
)
if echo "$result" | grep -q "communication error"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 15: Argument to known flag should not be treated as unknown option
echo ""
echo "Test 15: Format argument 'csv' not treated as unknown"
result=$(cat <<'EOF' | ./sensor-data list-errors -if csv 2>&1
sensor,value,sensor_id
ds18b20,85,001
EOF
)
# Should NOT contain "Unknown option" error
if echo "$result" | grep -q "Unknown option"; then
    echo "  ✗ FAIL - 'csv' was incorrectly treated as unknown option"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
else
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
fi

# Test 16: Date filter arguments not treated as unknown
echo ""
echo "Test 16: Date filter argument not treated as unknown"
result=$(echo '{"sensor":"ds18b20","value":"85","timestamp":"2026-01-15T10:00:00"}' | ./sensor-data list-errors --min-date 2026-01-01 2>&1)
if echo "$result" | grep -q "Unknown option"; then
    echo "  ✗ FAIL - Date argument incorrectly treated as unknown option"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
else
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
fi

# Test 17: Extension filter argument not treated as unknown
echo ""
echo "Test 17: Extension filter argument not treated as unknown"
cat <<'EOF' > /tmp/test_ext.out
{"sensor":"ds18b20","value":"85"}
EOF
result=$(./sensor-data list-errors -e .out /tmp/ 2>&1)
# Should not show "Unknown option" for ".out"
if echo "$result" | grep -q "Unknown option"; then
    echo "  ✗ FAIL - Extension argument incorrectly treated as unknown option"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
else
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
fi
rm -f /tmp/test_ext.out

# Test 18: Depth argument not treated as unknown
echo ""
echo "Test 18: Depth argument not treated as unknown"
mkdir -p /tmp/testdir_depth
echo '{"sensor":"ds18b20","value":"85"}' > /tmp/testdir_depth/test.out
result=$(./sensor-data list-errors -r -d 1 -e .out /tmp/testdir_depth/ 2>&1)
if echo "$result" | grep -q "Unknown option"; then
    echo "  ✗ FAIL - Depth argument incorrectly treated as unknown option"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
else
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
fi
rm -rf /tmp/testdir_depth

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
