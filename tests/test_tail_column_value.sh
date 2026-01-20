#!/bin/bash
# Integration tests for --tail-column-value filter

set -e  # Exit on error

echo "==========================================="
echo "Testing --tail-column-value functionality"
echo "==========================================="

# Build sensor-data if not already built
if [ ! -f sensor-data ]; then
    echo "Building sensor-data..."
    make sensor-data
fi

PASSED=0
FAILED=0

# Create temp directory for test files
TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

# Create test JSON file with multiple sensors
cat > "$TMPDIR/test.out" << 'EOF'
[{"sensor": "ds18b20", "value": 20.0, "sensor_id": "001"}]
[{"sensor": "dht22", "value": 50.0, "sensor_id": "002"}]
[{"sensor": "ds18b20", "value": 21.0, "sensor_id": "001"}]
[{"sensor": "ds18b20", "value": 22.0, "sensor_id": "001"}]
[{"sensor": "dht22", "value": 55.0, "sensor_id": "002"}]
[{"sensor": "ds18b20", "value": 23.0, "sensor_id": "001"}]
[{"sensor": "ds18b20", "value": 24.0, "sensor_id": "001"}]
[{"sensor": "dht22", "value": 60.0, "sensor_id": "002"}]
EOF

# Create test CSV file
cat > "$TMPDIR/test.csv" << 'EOF'
sensor,value,sensor_id
ds18b20,20.0,001
dht22,50.0,002
ds18b20,21.0,001
ds18b20,22.0,001
dht22,55.0,002
ds18b20,23.0,001
ds18b20,24.0,001
dht22,60.0,002
EOF

# Create larger test file for performance testing
cat > "$TMPDIR/large.out" << 'EOF'
[{"sensor": "ds18b20", "value": 1.0, "sensor_id": "001"}]
[{"sensor": "ds18b20", "value": 2.0, "sensor_id": "001"}]
[{"sensor": "ds18b20", "value": 3.0, "sensor_id": "001"}]
[{"sensor": "ds18b20", "value": 4.0, "sensor_id": "001"}]
[{"sensor": "ds18b20", "value": 5.0, "sensor_id": "001"}]
[{"sensor": "ds18b20", "value": 6.0, "sensor_id": "001"}]
[{"sensor": "ds18b20", "value": 7.0, "sensor_id": "001"}]
[{"sensor": "ds18b20", "value": 8.0, "sensor_id": "001"}]
[{"sensor": "ds18b20", "value": 9.0, "sensor_id": "001"}]
[{"sensor": "ds18b20", "value": 10.0, "sensor_id": "001"}]
EOF

echo ""
echo "=== Basic functionality tests ==="

# Test 1: Get last 2 rows where sensor=ds18b20 (JSON)
echo ""
echo "Test 1: Get last 2 rows where sensor=ds18b20 from JSON file"
result=$(./sensor-data transform --tail-column-value sensor:ds18b20 2 "$TMPDIR/test.out" --remove-whitespace)
# Should get rows with value 23.0 and 24.0
if echo "$result" | grep -q '"value":23' && echo "$result" | grep -q '"value":24'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected rows with value 23.0 and 24.0"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 2: Get last 1 row where sensor=dht22 (JSON)
echo ""
echo "Test 2: Get last 1 row where sensor=dht22 from JSON file"
result=$(./sensor-data transform --tail-column-value sensor:dht22 1 "$TMPDIR/test.out" --remove-whitespace)
# Should get row with value 60.0
if echo "$result" | grep -q '"value":60'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected row with value 60.0"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 3: Get last 2 rows where sensor=ds18b20 from CSV file
echo ""
echo "Test 3: Get last 2 rows where sensor=ds18b20 from CSV file"
result=$(./sensor-data transform --tail-column-value sensor:ds18b20 2 "$TMPDIR/test.csv" --remove-whitespace)
# Should get rows with value 23.0 and 24.0
if echo "$result" | grep -q '"value":"23.0"' && echo "$result" | grep -q '"value":"24.0"'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected rows with value 23.0 and 24.0"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 4: Count with --tail-column-value
echo ""
echo "Test 4: Count with --tail-column-value sensor:ds18b20 3"
result=$(./sensor-data count --tail-column-value sensor:ds18b20 3 "$TMPDIR/test.out")
if [ "$result" = "3" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 3"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 5: Count with --tail-column-value on CSV
echo ""
echo "Test 5: Count with --tail-column-value on CSV file"
result=$(./sensor-data count --tail-column-value sensor:dht22 2 "$TMPDIR/test.csv")
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

echo ""
echo "=== Edge case tests ==="

# Test 6: Request more rows than exist
echo ""
echo "Test 6: Request more rows than exist (n > matches)"
result=$(./sensor-data count --tail-column-value sensor:dht22 100 "$TMPDIR/test.out")
# Should return all 3 dht22 rows
if [ "$result" = "3" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 3 (all dht22 rows)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 7: No matching rows
echo ""
echo "Test 7: No matching rows (non-existent sensor)"
result=$(./sensor-data count --tail-column-value sensor:nonexistent 5 "$TMPDIR/test.out")
if [ "$result" = "0" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 0"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 8: n=1 (single row)
echo ""
echo "Test 8: Get exactly 1 row"
result=$(./sensor-data count --tail-column-value sensor:ds18b20 1 "$TMPDIR/test.out")
if [ "$result" = "1" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 1"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 9: Filter by sensor_id column
echo ""
echo "Test 9: Filter by sensor_id column"
result=$(./sensor-data count --tail-column-value sensor_id:002 2 "$TMPDIR/test.out")
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

echo ""
echo "=== Order preservation tests ==="

# Test 10: Verify output order is preserved (oldest first of the tail)
echo ""
echo "Test 10: Verify output order is preserved (oldest first in tail)"
result=$(./sensor-data transform --tail-column-value sensor:ds18b20 3 "$TMPDIR/large.out" --remove-whitespace)
# Should get values 8, 9, 10 in that order
line1=$(echo "$result" | head -1)
line2=$(echo "$result" | sed -n '2p')
line3=$(echo "$result" | tail -1)
if echo "$line1" | grep -q '"value":8' && \
   echo "$line2" | grep -q '"value":9' && \
   echo "$line3" | grep -q '"value":10'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected values 8, 9, 10 in order"
    echo "  Got:"
    echo "  Line 1: $line1"
    echo "  Line 2: $line2"
    echo "  Line 3: $line3"
    FAILED=$((FAILED + 1))
fi

echo ""
echo "=== Combination with other filters ==="

# Test 11: Combine with --remove-errors
echo ""
echo "Test 11: Combine --tail-column-value with --remove-errors"
# Create file with an error reading
cat > "$TMPDIR/with_errors.out" << 'EOF'
[{"sensor": "ds18b20", "value": 20.0, "sensor_id": "001"}]
[{"sensor": "ds18b20", "value": 85, "sensor_id": "001"}]
[{"sensor": "ds18b20", "value": 22.0, "sensor_id": "001"}]
[{"sensor": "ds18b20", "value": 23.0, "sensor_id": "001"}]
EOF
result=$(./sensor-data count --tail-column-value sensor:ds18b20 3 --remove-errors "$TMPDIR/with_errors.out")
# Should get 3 rows (excluding the error value 85)
if [ "$result" = "3" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 3 (after removing error)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 12: Combine with date filter
echo ""
echo "Test 12: Combine --tail-column-value with --min-date (check no conflict)"
# This test just verifies both options can be used together without error
result=$(./sensor-data count --tail-column-value sensor:ds18b20 2 --min-date 2020-01-01 "$TMPDIR/test.out" 2>&1) || true
if ! echo "$result" | grep -q -i "error"; then
    echo "  ✓ PASS (no error combining options)"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got error: $result"
    FAILED=$((FAILED + 1))
fi

echo ""
echo "=== Stats command tests ==="

# Test 13: Stats with --tail-column-value
echo ""
echo "Test 13: Stats with --tail-column-value"
result=$(./sensor-data stats --tail-column-value sensor:ds18b20 3 "$TMPDIR/large.out" 2>&1)
# Should calculate stats on values 8, 9, 10
if echo "$result" | grep -q "Count:.*3" && echo "$result" | grep -q "Min:.*8"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected stats on 3 values (8, 9, 10)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

echo ""
echo "=== Help text tests ==="

# Test 14: --tail-column-value appears in transform help
echo ""
echo "Test 14: --tail-column-value appears in transform --help"
result=$(./sensor-data transform --help 2>&1)
if echo "$result" | grep -q "tail-column-value"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  --tail-column-value not found in help"
    FAILED=$((FAILED + 1))
fi

# Test 15: --tail-column-value appears in count help
echo ""
echo "Test 15: --tail-column-value appears in count --help"
result=$(./sensor-data count --help 2>&1)
if echo "$result" | grep -q "tail-column-value"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  --tail-column-value not found in help"
    FAILED=$((FAILED + 1))
fi

# Test 16: --tail-column-value appears in stats help
echo ""
echo "Test 16: --tail-column-value appears in stats --help"
result=$(./sensor-data stats --help 2>&1)
if echo "$result" | grep -q "tail-column-value"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  --tail-column-value not found in help"
    FAILED=$((FAILED + 1))
fi

echo ""
echo "=== Error handling tests ==="

# Test 17: Missing column:value argument
echo ""
echo "Test 17: Error on missing column:value argument"
result=$(./sensor-data count --tail-column-value 2>&1) || true
if echo "$result" | grep -qi "error\|requires"; then
    echo "  ✓ PASS (error message shown)"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected error message"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 18: Missing count argument
echo ""
echo "Test 18: Error on missing count argument"
result=$(./sensor-data count --tail-column-value sensor:ds18b20 "$TMPDIR/test.out" 2>&1) || true
# The file path would be interpreted as the count, which should cause an error
if echo "$result" | grep -qi "error\|invalid"; then
    echo "  ✓ PASS (error message shown)"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected error message"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 19: Invalid column:value format (no colon)
echo ""
echo "Test 19: Error on invalid column:value format (no colon)"
result=$(./sensor-data count --tail-column-value sensorvalue 2 "$TMPDIR/test.out" 2>&1) || true
if echo "$result" | grep -qi "error\|format"; then
    echo "  ✓ PASS (error message shown)"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected error about format"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 20: Negative count value
echo ""
echo "Test 20: Error on negative count value"
result=$(./sensor-data count --tail-column-value sensor:ds18b20 -5 "$TMPDIR/test.out" 2>&1) || true
if echo "$result" | grep -qi "error\|positive"; then
    echo "  ✓ PASS (error message shown)"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected error about positive number"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 21: Zero count value
echo ""
echo "Test 21: Error on zero count value"
result=$(./sensor-data count --tail-column-value sensor:ds18b20 0 "$TMPDIR/test.out" 2>&1) || true
if echo "$result" | grep -qi "error\|positive"; then
    echo "  ✓ PASS (error message shown)"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected error about positive number"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

echo ""
echo "=== Latest command tests ==="

# Create file with timestamps for latest command
cat > "$TMPDIR/with_timestamps.out" << 'EOF'
[{"sensor": "ds18b20", "value": 20.0, "sensor_id": "001", "timestamp": 1700000000}]
[{"sensor": "dht22", "value": 50.0, "sensor_id": "002", "timestamp": 1700001000}]
[{"sensor": "ds18b20", "value": 21.0, "sensor_id": "001", "timestamp": 1700002000}]
[{"sensor": "ds18b20", "value": 22.0, "sensor_id": "003", "timestamp": 1700003000}]
[{"sensor": "dht22", "value": 55.0, "sensor_id": "002", "timestamp": 1700004000}]
EOF

# Test 22: Latest with --tail-column-value
echo ""
echo "Test 22: Latest with --tail-column-value"
result=$(./sensor-data latest --tail-column-value sensor:ds18b20 2 "$TMPDIR/with_timestamps.out" 2>&1) || true
# Should find latest for sensor_ids in the last 2 ds18b20 rows (001 and 003)
if echo "$result" | grep -q "001\|003"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected sensor_id 001 or 003"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

echo ""
echo "==========================================="
echo "Test Summary"
echo "==========================================="
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
