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
echo "Test 4: CSV input with -if csv"
result=$(cat <<'EOF' | ./sensor-data stats -if csv
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
# Test 12: Quartiles output
echo ""
echo "Test 12: Quartiles (Q1, Median, Q3, IQR)"
result=$(cat <<'EOF' | ./sensor-data stats
{"value":"10"}
{"value":"20"}
{"value":"30"}
{"value":"40"}
{"value":"50"}
EOF
)
if echo "$result" | grep -q "Quartiles:" && echo "$result" | grep -q "Q1 (25%):" && echo "$result" | grep -q "Median:" && echo "$result" | grep -q "Q3 (75%):" && echo "$result" | grep -q "IQR:"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected Quartiles section"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 13: Quartile values are correct
echo ""
echo "Test 13: Quartile values are correct"
result=$(cat <<'EOF' | ./sensor-data stats
{"value":"10"}
{"value":"20"}
{"value":"30"}
{"value":"40"}
{"value":"50"}
EOF
)
# For values 10,20,30,40,50: Q1=20, Median=30, Q3=40, IQR=20
if echo "$result" | grep -q "Q1 (25%):.*20" && echo "$result" | grep -q "Median:.*30" && echo "$result" | grep -q "Q3 (75%):.*40" && echo "$result" | grep -q "IQR:.*20"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Quartile values incorrect"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 14: Range calculation
echo ""
echo "Test 14: Range calculation"
result=$(cat <<'EOF' | ./sensor-data stats
{"value":"10"}
{"value":"50"}
EOF
)
if echo "$result" | grep -q "Range:.*40"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected Range: 40"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 15: Outlier detection (1.5*IQR method)
echo ""
echo "Test 15: Outlier detection"
result=$(cat <<'EOF' | ./sensor-data stats
{"value":"20"}
{"value":"21"}
{"value":"22"}
{"value":"23"}
{"value":"24"}
{"value":"85"}
EOF
)
# 85 should be detected as outlier (well beyond 1.5*IQR)
if echo "$result" | grep -q "Outliers" && echo "$result" | grep -q "Count:.*1"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 1 outlier detected"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 16: Outlier percentage
echo ""
echo "Test 16: Outlier percentage"
result=$(cat <<'EOF' | ./sensor-data stats
{"value":"20"}
{"value":"21"}
{"value":"22"}
{"value":"23"}
{"value":"24"}
{"value":"85"}
{"value":"86"}
{"value":"87"}
{"value":"88"}
{"value":"89"}
EOF
)
# Multiple outliers should show percentage
if echo "$result" | grep -q "Percent:.*[0-9]"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected outlier percentage"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 17: Delta stats section exists
echo ""
echo "Test 17: Delta stats section"
result=$(cat <<'EOF' | ./sensor-data stats
{"value":"10"}
{"value":"15"}
{"value":"25"}
{"value":"30"}
EOF
)
if echo "$result" | grep -q "Delta (consecutive changes):" && echo "$result" | grep -q "Min:" && echo "$result" | grep -q "Max:" && echo "$result" | grep -q "Mean:"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected Delta stats section"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 18: Delta stats values
echo ""
echo "Test 18: Delta stats values correct"
result=$(cat <<'EOF' | ./sensor-data stats
{"value":"10"}
{"value":"15"}
{"value":"25"}
{"value":"30"}
EOF
)
# Deltas: 5, 10, 5 -> Min=5, Max=10, Mean=6.67
if echo "$result" | grep -q "Delta" && echo "$result" | grep "Min:" | tail -1 | grep -q "5"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Delta min should be 5"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 19: No delta section for single value
echo ""
echo "Test 19: No delta section for single value"
result=$(cat <<'EOF' | ./sensor-data stats
{"value":"42"}
EOF
)
if ! echo "$result" | grep -q "Delta (consecutive changes):"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Should not show Delta section for single value"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 20: No outliers detected
echo ""
echo "Test 20: No outliers in uniform data"
result=$(cat <<'EOF' | ./sensor-data stats
{"value":"20"}
{"value":"21"}
{"value":"22"}
{"value":"23"}
{"value":"24"}
EOF
)
if echo "$result" | grep -q "Outliers" && echo "$result" | grep "Count:" | tail -1 | grep -q "0"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 0 outliers in uniform data"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 21: --only-value filter
echo ""
echo "Test 21: --only-value filter"
result=$(cat <<'EOF' | ./sensor-data stats --only-value sensor:ds18b20
{"sensor":"ds18b20","value":"22.5"}
{"sensor":"dht22","value":"65.0"}
{"sensor":"ds18b20","value":"23.0"}
{"sensor":"bme280","value":"1013.25"}
{"sensor":"ds18b20","value":"24.5"}
EOF
)
# Should only include 3 ds18b20 readings
if echo "$result" | grep -q "Count:.*3"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 3 readings (only ds18b20)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 22: --exclude-value filter
echo ""
echo "Test 22: --exclude-value filter"
result=$(cat <<'EOF' | ./sensor-data stats --exclude-value sensor:ds18b20
{"sensor":"ds18b20","value":"22.5"}
{"sensor":"dht22","value":"65.0"}
{"sensor":"ds18b20","value":"23.0"}
{"sensor":"bme280","value":"1013.25"}
{"sensor":"ds18b20","value":"24.5"}
EOF
)
# Should only include 2 readings (dht22 and bme280)
if echo "$result" | grep -q "Count:.*2"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 readings (excluding ds18b20)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 23: Multiple --only-value filters (AND logic)
echo ""
echo "Test 23: Multiple --only-value filters"
result=$(cat <<'EOF' | ./sensor-data stats --only-value sensor:ds18b20 --only-value unit:C
{"sensor":"ds18b20","value":"22.5","unit":"C"}
{"sensor":"ds18b20","value":"85","unit":"F"}
{"sensor":"dht22","value":"65.0","unit":"C"}
{"sensor":"ds18b20","value":"23.0","unit":"C"}
EOF
)
# Should only include ds18b20 readings with unit:C (2 readings)
if echo "$result" | grep -q "Count:.*2"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 readings (ds18b20 with unit:C)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 24: --remove-errors filter
echo ""
echo "Test 24: --remove-errors filter"
result=$(cat <<'EOF' | ./sensor-data stats --remove-errors
{"sensor":"ds18b20","value":"22.5"}
{"sensor":"ds18b20","value":"85"}
{"sensor":"ds18b20","value":"-127"}
{"sensor":"ds18b20","value":"23.0"}
EOF
)
# Should exclude 85 and -127, leaving 2 readings
if echo "$result" | grep -q "Count:.*2"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 readings (excluding errors 85 and -127)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 25: --not-empty filter
echo ""
echo "Test 25: --not-empty filter"
result=$(cat <<'EOF' | ./sensor-data stats --not-empty value
{"sensor":"ds18b20","value":"22.5"}
{"sensor":"ds18b20","value":""}
{"sensor":"ds18b20","value":"23.0"}
EOF
)
# Should exclude empty value, leaving 2 readings
if echo "$result" | grep -q "Count:.*2"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 readings (excluding empty value)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 26: --clean filter (combines --remove-empty-json --not-empty value --remove-errors)
echo ""
echo "Test 26: --clean filter"
result=$(cat <<'EOF' | ./sensor-data stats --clean
{"sensor":"ds18b20","value":"22.5"}
{"sensor":"ds18b20","value":""}
{"sensor":"ds18b20","value":"85"}
{"sensor":"ds18b20","value":"23.0"}
{"sensor":"ds18b20","value":"-127"}
EOF
)
# Should exclude empty, 85, and -127, leaving 2 readings
if echo "$result" | grep -q "Count:.*2"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 readings (--clean excludes empty and errors)"
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
