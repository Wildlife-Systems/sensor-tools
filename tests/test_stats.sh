#!/bin/bash
# Integration tests for stats functionality

set -e  # Exit on error

echo "================================"
echo "Testing stats functionality"
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

# Test: stats --help shows usage
echo "Test: stats --help shows usage"
result=$(./sensor-data stats --help 2>&1) || true
if echo "$result" | grep -q -i "stats\|usage"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected stats help text"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 1: Basic statistics
echo "Test 1: Basic statistics (default: value column only)"
result=$(cat <<'EOF' | ./sensor-data stats
{"sensor":"ds18b20","value":"22.5","sensor_id":"001"}
{"sensor":"ds18b20","value":"23.0","sensor_id":"002"}
{"sensor":"ds18b20","value":"21.5","sensor_id":"003"}
{"sensor":"ds18b20","value":"24.0","sensor_id":"004"}
{"sensor":"ds18b20","value":"22.0","sensor_id":"005"}
EOF
)
if echo "$result" | grep -q "value:" && echo "$result" | grep -q "Count:.*5" && echo "$result" | grep -q "Min:.*21.5" && echo "$result" | grep -q "Max:.*24" && ! echo "$result" | grep -q "sensor_id:"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 2: Column filter
echo ""
echo "Test 2: Column filter (-c value)"
result=$(cat <<'EOF' | ./sensor-data stats -c value
{"sensor":"ds18b20","value":"22.5","sensor_id":"001"}
{"sensor":"ds18b20","value":"23.0","sensor_id":"002"}
{"sensor":"ds18b20","value":"21.5","sensor_id":"003"}
EOF
)
if echo "$result" | grep -q "value:" && echo "$result" | grep -q "Count:.*3" && ! echo "$result" | grep -q "sensor_id:"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 3: No numeric data
echo ""
echo "Test 3: No numeric data"
result=$(cat <<'EOF' | ./sensor-data stats
{"sensor":"ds18b20","value":"invalid","sensor_id":"abc"}
{"sensor":"ds18b20","value":"error","sensor_id":"def"}
EOF
)
if echo "$result" | grep -q "No numeric data found"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 4: CSV input
echo ""
echo "Test 4: CSV input with -f csv"
result=$(cat <<'EOF' | ./sensor-data stats -f csv
sensor,value,sensor_id
ds18b20,22.5,001
ds18b20,23.0,002
ds18b20,21.5,003
EOF
)
if echo "$result" | grep -q "value:" && echo "$result" | grep -q "Count:.*3"; then
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
cat <<'EOF' > /tmp/test_stats.out
{"sensor":"ds18b20","value":"10.0","sensor_id":"001"}
{"sensor":"ds18b20","value":"20.0","sensor_id":"002"}
{"sensor":"ds18b20","value":"30.0","sensor_id":"003"}
EOF
result=$(./sensor-data stats /tmp/test_stats.out)
if echo "$result" | grep -q "value:" && echo "$result" | grep -q "Min:.*10" && echo "$result" | grep -q "Max:.*30" && echo "$result" | grep -q "Mean:.*20"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi
rm -f /tmp/test_stats.out

# Test 6: Median calculation (odd count)
echo ""
echo "Test 6: Median calculation (odd count)"
result=$(cat <<'EOF' | ./sensor-data stats -c value
{"sensor":"ds18b20","value":"1"}
{"sensor":"ds18b20","value":"2"}
{"sensor":"ds18b20","value":"3"}
{"sensor":"ds18b20","value":"4"}
{"sensor":"ds18b20","value":"5"}
EOF
)
if echo "$result" | grep -q "Median:.*3"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 7: Median calculation (even count)
echo ""
echo "Test 7: Median calculation (even count)"
result=$(cat <<'EOF' | ./sensor-data stats -c value
{"sensor":"ds18b20","value":"1"}
{"sensor":"ds18b20","value":"2"}
{"sensor":"ds18b20","value":"3"}
{"sensor":"ds18b20","value":"4"}
EOF
)
if echo "$result" | grep -q "Median:.*2.5"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 8: Mixed numeric and non-numeric
echo ""
echo "Test 8: Mixed numeric and non-numeric values"
result=$(cat <<'EOF' | ./sensor-data stats
{"sensor":"ds18b20","value":"22.5","sensor_id":"001"}
{"sensor":"ds18b20","value":"invalid","sensor_id":"002"}
{"sensor":"ds18b20","value":"23.0","sensor_id":"003"}
EOF
)
if echo "$result" | grep -q "value:" && echo "$result" | grep -q "Count:.*2"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 9: Multiple columns
echo ""
echo "Test 9: Analyze specific non-default column"
result=$(cat <<'EOF' | ./sensor-data stats -c temperature
{"temperature":"22.5","humidity":"65.0","sensor_id":"001"}
{"temperature":"23.0","humidity":"64.5","sensor_id":"002"}
EOF
)
if echo "$result" | grep -q "temperature:" && ! echo "$result" | grep -q "humidity:" && ! echo "$result" | grep -q "sensor_id:"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 10: Missing value column
echo ""
echo "Test 10: No value column present (should show no data)"
result=$(cat <<'EOF' | ./sensor-data stats
{"temperature":"22.5","humidity":"65.0","sensor_id":"001"}
{"temperature":"23.0","humidity":"64.5","sensor_id":"002"}
EOF
)
if echo "$result" | grep -q "No numeric data found"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 11: Empty value column
echo ""
echo "Test 11: Empty or missing value field"
result=$(cat <<'EOF' | ./sensor-data stats
{"sensor":"ds18b20","value":"","sensor_id":"001"}
{"sensor":"ds18b20","sensor_id":"002"}
{"sensor":"ds18b20","value":"22.5","sensor_id":"003"}
EOF
)
if echo "$result" | grep -q "value:" && echo "$result" | grep -q "Count:.*1"; then
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
    echo "✓ All stats tests passed!"
    exit 0
else
    echo "✗ Some tests failed"
    exit 1
fi
