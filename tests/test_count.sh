#!/bin/bash
# Integration tests for count command

set -e  # Exit on error

echo "================================"
echo "Testing count functionality"
echo "================================"

# Build sensor-data if not already built
if [ ! -f sensor-data ]; then
    echo "Building sensor-data..."
    make sensor-data
fi

PASSED=0
FAILED=0

# Test: count --help shows help
echo "Test: count --help shows count usage"
result=$(./sensor-data count --help 2>&1) || true
if echo "$result" | grep -q -i "count"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected count help text"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count single reading
echo ""
echo "Test: Count single JSON reading"
input='[{"sensor": "ds18b20", "value": 22.5}]'
result=$(echo "$input" | ./sensor-data count)
if [ "$result" = "1" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 1"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count multiple readings in one line
echo ""
echo "Test: Count multiple readings in single JSON array"
input='[{"sensor": "a", "value": 1}, {"sensor": "b", "value": 2}, {"sensor": "c", "value": 3}]'
result=$(echo "$input" | ./sensor-data count)
if [ "$result" = "3" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 3"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count multiple lines
echo ""
echo "Test: Count readings across multiple lines"
input='[{"sensor": "a", "value": 1}]
[{"sensor": "b", "value": 2}]
[{"sensor": "c", "value": 3}]'
result=$(echo "$input" | ./sensor-data count)
if [ "$result" = "3" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 3"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count with empty input
echo ""
echo "Test: Count with no readings returns 0"
result=$(echo "" | ./sensor-data count)
if [ "$result" = "0" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 0"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count with --remove-empty-json
echo ""
echo "Test: --remove-empty-json ignores empty arrays"
input='[{}]
[]
[{"sensor": "a", "value": 1}]'
result=$(echo "$input" | ./sensor-data count --remove-empty-json)
if [ "$result" = "1" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 1"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count without --remove-empty-json includes empty objects
echo ""
echo "Test: Without --remove-empty-json, empty arrays still count as 0"
input='[{}]
[]
[{"sensor": "a", "value": 1}]'
result=$(echo "$input" | ./sensor-data count)
if [ "$result" = "1" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 1"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count with --remove-errors
echo ""
echo "Test: --remove-errors excludes DS18B20 error readings"
input='[{"sensor": "ds18b20", "value": "85"}]
[{"sensor": "ds18b20", "value": "-127"}]
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "ds18b20", "value": "23.0"}]'
result=$(echo "$input" | ./sensor-data count --remove-errors)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count with --only-value filter
echo ""
echo "Test: --only-value filters by column value"
input='[{"type": "ds18b20", "value": 22.5}]
[{"type": "bme280", "value": 23.0}]
[{"type": "ds18b20", "value": 24.0}]'
result=$(echo "$input" | ./sensor-data count --only-value type:ds18b20)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count with --exclude-value filter
echo ""
echo "Test: --exclude-value excludes by column value"
input='[{"type": "ds18b20", "value": 22.5}]
[{"type": "bme280", "value": 23.0}]
[{"type": "ds18b20", "value": 24.0}]'
result=$(echo "$input" | ./sensor-data count --exclude-value type:bme280)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count with --not-empty filter
echo ""
echo "Test: --not-empty filters rows with empty columns"
input='[{"type": "ds18b20", "value": 22.5}]
[{"type": "ds18b20", "value": ""}]
[{"type": "ds18b20"}]'
result=$(echo "$input" | ./sensor-data count --not-empty value)
if [ "$result" = "1" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 1"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count with CSV input
echo ""
echo "Test: Count with CSV input format"
input='type,value
ds18b20,22.5
ds18b20,23.0
bme280,24.0'
result=$(echo "$input" | ./sensor-data count -f csv)
if [ "$result" = "3" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 3"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count with CSV input and filter
echo ""
echo "Test: Count with CSV input and --only-value filter"
input='type,value
ds18b20,22.5
ds18b20,23.0
bme280,24.0'
result=$(echo "$input" | ./sensor-data count -f csv --only-value type:ds18b20)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count combining multiple filters
echo ""
echo "Test: Count with multiple filters combined"
input='[{"type": "ds18b20", "location": "indoor", "value": 22.5}]
[{"type": "ds18b20", "location": "outdoor", "value": 23.0}]
[{"type": "bme280", "location": "indoor", "value": 24.0}]
[{"type": "ds18b20", "location": "indoor", "value": 25.0}]'
result=$(echo "$input" | ./sensor-data count --only-value type:ds18b20 --only-value location:indoor)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Summary
echo ""
echo "================================"
echo "Count Tests Summary"
echo "================================"
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo ""

if [ $FAILED -gt 0 ]; then
    echo "SOME TESTS FAILED"
    exit 1
else
    echo "ALL TESTS PASSED"
    exit 0
fi
