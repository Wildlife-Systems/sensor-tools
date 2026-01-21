#!/bin/bash
# Test script for 'sensor-data distinct' command

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SENSOR_DATA="${SCRIPT_DIR}/../sensor-data"

# Check if sensor-data exists
if [ ! -x "$SENSOR_DATA" ]; then
    echo "Error: sensor-data not found at $SENSOR_DATA"
    exit 1
fi

PASSED=0
FAILED=0

# Helper function to run a test
run_test() {
    local name="$1"
    local expected="$2"
    local actual="$3"
    
    if [ "$expected" = "$actual" ]; then
        echo "PASS: $name"
        ((PASSED++))
    else
        echo "FAIL: $name"
        echo "  Expected: $expected"
        echo "  Actual:   $actual"
        ((FAILED++))
    fi
}

echo "=== Testing distinct command ==="

# Test 1: Basic distinct values
input='{"sensor": "temp1", "value": 23}
{"sensor": "temp2", "value": 24}
{"sensor": "temp1", "value": 25}
{"sensor": "humidity", "value": 60}'

result=$(echo "$input" | $SENSOR_DATA distinct sensor | sort | tr '\n' ' ' | sed 's/ $//')
run_test "Basic distinct values" "humidity temp1 temp2" "$result"

# Test 2: Distinct with counts
result=$(echo "$input" | $SENSOR_DATA distinct sensor -c | head -1 | cut -f1)
run_test "Distinct with counts (top count)" "2" "$result"

# Test 3: Distinct with --only-value filter
input_with_filter='{"sensor": "temp1", "internal": "false"}
{"sensor": "temp2", "internal": "true"}
{"sensor": "temp1", "internal": "false"}
{"sensor": "humidity", "internal": "false"}'

result=$(echo "$input_with_filter" | $SENSOR_DATA distinct sensor --only-value internal:false | sort | tr '\n' ' ' | sed 's/ $//')
run_test "Distinct with --only-value filter" "humidity temp1" "$result"

# Test 4: Distinct with --exclude-value filter
result=$(echo "$input_with_filter" | $SENSOR_DATA distinct sensor --exclude-value sensor:temp1 | sort | tr '\n' ' ' | sed 's/ $//')
run_test "Distinct with --exclude-value filter" "humidity temp2" "$result"

# Test 5: CSV output format
result=$(echo "$input" | $SENSOR_DATA distinct sensor -of csv | head -1)
run_test "CSV output format header" "value" "$result"

# Test 6: CSV output with counts
result=$(echo "$input" | $SENSOR_DATA distinct sensor -of csv -c | head -1)
run_test "CSV output with counts header" "value,count" "$result"

# Test 7: JSON output format
result=$(echo "$input" | $SENSOR_DATA distinct sensor -of json | head -1)
run_test "JSON output format starts with [" "[" "$result"

# Test 8: Missing column returns empty
result=$(echo '{"other": "value"}' | $SENSOR_DATA distinct sensor | wc -l | tr -d ' ')
run_test "Missing column returns no values" "0" "$result"

# Test 9: Empty values are excluded
input_empty='{"sensor": "temp1"}
{"sensor": ""}
{"sensor": "temp2"}'

result=$(echo "$input_empty" | $SENSOR_DATA distinct sensor | sort | tr '\n' ' ' | sed 's/ $//')
run_test "Empty values are excluded" "temp1 temp2" "$result"

# Test 10: Distinct on value column
result=$(echo '{"sensor": "temp", "value": "10"}
{"sensor": "temp", "value": "20"}
{"sensor": "temp", "value": "10"}' | $SENSOR_DATA distinct value | sort | tr '\n' ' ' | sed 's/ $//')
run_test "Distinct on value column" "10 20" "$result"

echo ""
echo "=== Results ==="
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ $FAILED -gt 0 ]; then
    exit 1
fi
