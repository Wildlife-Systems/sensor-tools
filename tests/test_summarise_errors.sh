#!/bin/bash
# Integration tests for summarise-errors functionality

set -e  # Exit on error

echo "================================"
echo "Testing summarise-errors functionality"
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

# Test: summarise-errors --help shows usage
echo "Test: summarise-errors --help shows usage"
result=$(./sensor-data summarise-errors --help 2>&1) || true
if echo "$result" | grep -q -i "summari\|error\|usage"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected summarise-errors help text"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 1: JSON stdin with multiple error types
echo "Test 1: JSON stdin with multiple error types"
result=$(cat <<'EOF' | ./sensor-data summarise-errors
{"sensor":"ds18b20","value":"22.5","sensor_id":"001"}
{"sensor":"ds18b20","value":"85","sensor_id":"002"}
{"sensor":"ds18b20","value":"-127","sensor_id":"003"}
{"sensor":"ds18b20","value":"85","sensor_id":"004"}
{"sensor":"ds18b20","value":"85","sensor_id":"005"}
{"sensor":"ds18b20","value":"-127","sensor_id":"006"}
{"sensor":"ds18b20","value":"23.0","sensor_id":"007"}
EOF
)
if echo "$result" | grep -q "communication error.*3 occurrence" && echo "$result" | grep -q "disconnected.*2 occurrence" && echo "$result" | grep -q "Total errors: 5"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 2: JSON stdin with only one error type
echo ""
echo "Test 2: JSON stdin with single error type"
result=$(cat <<'EOF' | ./sensor-data summarise-errors
{"sensor":"ds18b20","value":"85","sensor_id":"001"}
{"sensor":"ds18b20","value":"85","sensor_id":"002"}
{"sensor":"ds18b20","value":"85","sensor_id":"003"}
EOF
)
if echo "$result" | grep -q "communication error.*3 occurrence" && echo "$result" | grep -q "Total errors: 3"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 3: No errors
echo ""
echo "Test 3: No errors found"
result=$(cat <<'EOF' | ./sensor-data summarise-errors
{"sensor":"ds18b20","value":"22.5","sensor_id":"001"}
{"sensor":"ds18b20","value":"23.0","sensor_id":"002"}
EOF
)
if echo "$result" | grep -q "No errors found"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 4: CSV stdin
echo ""
echo "Test 4: CSV stdin with -f csv flag"
result=$(cat <<'EOF' | ./sensor-data summarise-errors -f csv
sensor,value,sensor_id
ds18b20,85,001
ds18b20,-127,002
ds18b20,85,003
ds18b20,22.5,004
EOF
)
if echo "$result" | grep -q "communication error.*2 occurrence" && echo "$result" | grep -q "disconnected.*1 occurrence" && echo "$result" | grep -q "Total errors: 3"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 5: File input
echo ""
echo "Test 5: File input"
cat <<'EOF' > /tmp/test_summarise.out
{"sensor":"ds18b20","value":"85","sensor_id":"001"}
{"sensor":"ds18b20","value":"-127","sensor_id":"002"}
{"sensor":"ds18b20","value":"85","sensor_id":"003"}
{"sensor":"ds18b20","value":"22.5","sensor_id":"004"}
{"sensor":"ds18b20","value":"-127","sensor_id":"005"}
{"sensor":"ds18b20","value":"-127","sensor_id":"006"}
EOF
result=$(./sensor-data summarise-errors /tmp/test_summarise.out)
if echo "$result" | grep -q "communication error.*2 occurrence" && echo "$result" | grep -q "disconnected.*3 occurrence" && echo "$result" | grep -q "Total errors: 5"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi
rm -f /tmp/test_summarise.out

# Test 6: Piped input
echo ""
echo "Test 6: Piped input from echo"
result=$(echo '{"sensor":"ds18b20","value":"85","sensor_id":"001"}' | ./sensor-data summarise-errors)
if echo "$result" | grep -q "communication error.*1 occurrence" && echo "$result" | grep -q "Total errors: 1"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 7: Large counts (plural handling)
echo ""
echo "Test 7: Large counts"
result=$(cat <<'EOF' | ./sensor-data summarise-errors
{"sensor":"ds18b20","value":"85","sensor_id":"001"}
{"sensor":"ds18b20","value":"85","sensor_id":"002"}
{"sensor":"ds18b20","value":"85","sensor_id":"003"}
{"sensor":"ds18b20","value":"85","sensor_id":"004"}
{"sensor":"ds18b20","value":"85","sensor_id":"005"}
{"sensor":"ds18b20","value":"85","sensor_id":"006"}
{"sensor":"ds18b20","value":"85","sensor_id":"007"}
{"sensor":"ds18b20","value":"85","sensor_id":"008"}
{"sensor":"ds18b20","value":"85","sensor_id":"009"}
{"sensor":"ds18b20","value":"85","sensor_id":"010"}
EOF
)
if echo "$result" | grep -q "communication error.*10 occurrences" && echo "$result" | grep -q "Total errors: 10"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 8: Temperature field instead of value
echo ""
echo "Test 8: Temperature field"
result=$(cat <<'EOF' | ./sensor-data summarise-errors
{"sensor":"ds18b20","temperature":"85","sensor_id":"001"}
{"sensor":"ds18b20","temperature":"-127","sensor_id":"002"}
EOF
)
if echo "$result" | grep -q "communication error.*temperature=85.*1 occurrence" && echo "$result" | grep -q "disconnected.*temperature=-127.*1 occurrence"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 9: Non-DS18B20 sensor (should not count)
echo ""
echo "Test 9: Non-DS18B20 sensor"
result=$(cat <<'EOF' | ./sensor-data summarise-errors
{"sensor":"dht22","value":"85","sensor_id":"001"}
{"sensor":"bme280","value":"85","sensor_id":"002"}
EOF
)
if echo "$result" | grep -q "No errors found"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Summary
echo ""
echo "================================"
echo "Test Summary"
echo "================================"
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ $FAILED -eq 0 ]; then
    echo "✓ All summarise-errors tests passed!"
    exit 0
else
    echo "✗ Some tests failed"
    exit 1
fi
