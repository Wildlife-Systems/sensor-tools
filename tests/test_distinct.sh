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

# Test 11: Distinct with --tail-column-value filter
input_with_types='{"sensor": "temp1", "type": "ds18b20"}
{"sensor": "temp2", "type": "dht22"}
{"sensor": "temp3", "type": "ds18b20"}
{"sensor": "temp4", "type": "ds18b20"}
{"sensor": "temp5", "type": "dht22"}'

result=$(echo "$input_with_types" | $SENSOR_DATA distinct sensor --tail-column-value type:ds18b20 2 | sort | tr '\n' ' ' | sed 's/ $//')
run_test "Distinct with --tail-column-value filter" "temp3 temp4" "$result"

# Test 12: Distinct with CSV input format
csv_input='sensor,value,type
temp1,22.5,ds18b20
temp2,45.0,dht22
temp1,23.0,ds18b20'

result=$(echo "$csv_input" | $SENSOR_DATA distinct sensor -if csv | sort | tr '\n' ' ' | sed 's/ $//')
run_test "Distinct with CSV input" "temp1 temp2" "$result"

# Test 13: Distinct with date filtering --min-date
input_dated='{"sensor": "temp1", "timestamp": "2024-01-01T00:00:00"}
{"sensor": "temp2", "timestamp": "2024-06-01T00:00:00"}
{"sensor": "temp3", "timestamp": "2024-12-01T00:00:00"}'

result=$(echo "$input_dated" | $SENSOR_DATA distinct sensor --min-date 2024-05-01 | sort | tr '\n' ' ' | sed 's/ $//')
run_test "Distinct with --min-date filter" "temp2 temp3" "$result"

# Test 14: Distinct with date filtering --max-date
result=$(echo "$input_dated" | $SENSOR_DATA distinct sensor --max-date 2024-07-01 | sort | tr '\n' ' ' | sed 's/ $//')
run_test "Distinct with --max-date filter" "temp1 temp2" "$result"

# Test 15: Distinct with combined date range
result=$(echo "$input_dated" | $SENSOR_DATA distinct sensor --min-date 2024-05-01 --max-date 2024-07-01 | tr '\n' ' ' | sed 's/ $//')
run_test "Distinct with date range filter" "temp2" "$result"

# Test 16: Distinct with --remove-errors
input_errors='{"sensor": "ds18b20", "value": "22.5"}
{"sensor": "ds18b20", "value": "85"}
{"sensor": "other", "value": "30"}'

result=$(echo "$input_errors" | $SENSOR_DATA distinct sensor --remove-errors | sort | tr '\n' ' ' | sed 's/ $//')
run_test "Distinct with --remove-errors" "ds18b20 other" "$result"

# Test 17: Distinct --help shows usage
result=$($SENSOR_DATA distinct --help 2>&1)
if echo "$result" | grep -qi "distinct"; then
    echo "PASS: distinct --help shows usage"
    ((PASSED++))
else
    echo "FAIL: distinct --help shows usage"
    ((FAILED++))
fi

# Test 18: Distinct with empty input returns empty
result=$(echo "" | $SENSOR_DATA distinct sensor 2>/dev/null | wc -l | tr -d ' ')
run_test "Distinct with empty input" "0" "$result"

# Test 19: Distinct with --unique filter
input_dup='{"sensor": "temp1", "value": "22.5"}
{"sensor": "temp1", "value": "22.5"}
{"sensor": "temp2", "value": "23.0"}'

result=$(echo "$input_dup" | $SENSOR_DATA distinct sensor --unique | sort | tr '\n' ' ' | sed 's/ $//')
run_test "Distinct with --unique (removes duplicate rows first)" "temp1 temp2" "$result"

# Test 20: Distinct column with numeric values
input_numeric='{"sensor": "temp", "value": 100}
{"sensor": "temp", "value": 200}
{"sensor": "temp", "value": 100}'

result=$(echo "$input_numeric" | $SENSOR_DATA distinct value | sort -n | tr '\n' ' ' | sed 's/ $//')
run_test "Distinct with numeric values" "100 200" "$result"

echo ""
echo "=== Results ==="
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ $FAILED -gt 0 ]; then
    exit 1
fi
