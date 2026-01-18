#!/bin/bash
# Integration tests for transform command with various options

set -e  # Exit on error

echo "================================"
echo "Testing transform functionality"
echo "================================"

# Build sensor-data if not already built
if [ ! -f sensor-data ]; then
    echo "Building sensor-data..."
    make sensor-data
fi

PASSED=0
FAILED=0

# Test: No arguments shows help
echo "Test: No arguments shows help"
result=$(./sensor-data 2>&1) || true
if echo "$result" | grep -q -i "usage\|Usage\|USAGE"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected usage/help text"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Unknown command shows help
echo ""
echo "Test: Unknown command shows help"
result=$(./sensor-data unknown-command 2>&1) || true
if echo "$result" | grep -q -i "usage\|Usage\|USAGE\|Unknown command"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected usage/help or unknown command message"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: help flag shows help
echo ""
echo "Test: --help flag shows help"
result=$(./sensor-data --help 2>&1) || true
if echo "$result" | grep -q -i "usage\|Usage\|USAGE"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected usage/help text"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: transform --help shows transform help
echo ""
echo "Test: transform --help shows transform usage"
result=$(./sensor-data transform --help 2>&1) || true
if echo "$result" | grep -q -i "transform"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected transform help text"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 1: JSON passthrough (no filtering)
echo "Test 1: JSON passthrough (no filtering)"
input='[ { "sensor": "ds18b20", "value": 22.5 } ]'
result=$(echo "$input" | ./sensor-data transform)
if [ "$result" = "$input" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: $input"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 1a: --remove-whitespace for compact JSON output
echo ""
echo "Test 1a: --remove-whitespace for compact JSON output"
result=$(echo '{ "sensor": "ds18b20", "value": 22.5 }' | ./sensor-data transform --remove-whitespace)
expected='[{"sensor":"ds18b20","value":22.5}]'
if [ "$result" = "$expected" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: $expected"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 1b: --remove-whitespace with multiple objects
echo ""
echo "Test 1b: --remove-whitespace with multiple objects"
result=$(echo '[ { "sensor": "ds18b20", "value": 22.5 }, { "sensor": "dht22", "value": 45 } ]' | ./sensor-data transform --remove-whitespace)
# Check that it has no spaces after colons or commas within objects
if echo "$result" | grep -q '"sensor":"ds18b20"' && ! echo "$result" | grep -q ': '; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected compact JSON without spaces"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 2: JSON to CSV conversion
echo ""
echo "Test 2: JSON to CSV conversion"
result=$(echo '{"sensor":"ds18b20","value":"22.5"}' | ./sensor-data transform -of csv)
if echo "$result" | grep -q "sensor,value" && echo "$result" | grep -q "ds18b20,22.5"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 3: Date filtering --min-date
echo ""
echo "Test 3: Date filtering --min-date"
result=$(cat <<'EOF' | ./sensor-data transform --min-date 2026-01-15
{"timestamp":"2026-01-10T10:00:00","sensor":"ds18b20","value":"20.0"}
{"timestamp":"2026-01-15T10:00:00","sensor":"ds18b20","value":"21.0"}
{"timestamp":"2026-01-20T10:00:00","sensor":"ds18b20","value":"22.0"}
EOF
)
count=$(echo "$result" | grep -c "ds18b20" || true)
if [ "$count" -eq 2 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 lines, got $count"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 4: Date filtering --max-date
echo ""
echo "Test 4: Date filtering --max-date"
result=$(cat <<'EOF' | ./sensor-data transform --max-date 2026-01-15
{"timestamp":"2026-01-10T10:00:00","sensor":"ds18b20","value":"20.0"}
{"timestamp":"2026-01-15T10:00:00","sensor":"ds18b20","value":"21.0"}
{"timestamp":"2026-01-20T10:00:00","sensor":"ds18b20","value":"22.0"}
EOF
)
count=$(echo "$result" | grep -c "ds18b20" || true)
if [ "$count" -eq 2 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 lines, got $count"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 5: Date filtering --min-date and --max-date combined
echo ""
echo "Test 5: Date filtering with both min and max"
result=$(cat <<'EOF' | ./sensor-data transform --min-date 2026-01-12 --max-date 2026-01-18
{"timestamp":"2026-01-10T10:00:00","sensor":"ds18b20","value":"20.0"}
{"timestamp":"2026-01-15T10:00:00","sensor":"ds18b20","value":"21.0"}
{"timestamp":"2026-01-20T10:00:00","sensor":"ds18b20","value":"22.0"}
EOF
)
count=$(echo "$result" | grep -c "ds18b20" || true)
if [ "$count" -eq 1 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 1 line, got $count"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 6: Remove errors (--remove-errors)
echo ""
echo "Test 6: Remove errors (--remove-errors)"
result=$(cat <<'EOF' | ./sensor-data transform --remove-errors
{"sensor":"ds18b20","value":"22.5"}
{"sensor":"ds18b20","value":"85"}
{"sensor":"ds18b20","value":"23.0"}
{"sensor":"ds18b20","value":"-127"}
EOF
)
count=$(echo "$result" | grep -c "ds18b20" || true)
if [ "$count" -eq 2 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 lines (errors removed), got $count"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 7: Only value filter (--only-value)
echo ""
echo "Test 7: Only value filter (--only-value)"
result=$(cat <<'EOF' | ./sensor-data transform --only-value value:22.5
{"sensor":"ds18b20","value":"22.5"}
{"sensor":"ds18b20","value":"23.0"}
{"sensor":"ds18b20","value":"22.5"}
EOF
)
count=$(echo "$result" | grep -c "22.5" || true)
if [ "$count" -eq 2 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 lines with value=22.5, got $count"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 8: Not empty filter (--not-empty)
echo ""
echo "Test 8: Not empty filter (--not-empty)"
result=$(cat <<'EOF' | ./sensor-data transform --not-empty sensor_id
{"sensor":"ds18b20","value":"22.5","sensor_id":"001"}
{"sensor":"ds18b20","value":"23.0"}
{"sensor":"ds18b20","value":"24.0","sensor_id":""}
{"sensor":"ds18b20","value":"25.0","sensor_id":"002"}
EOF
)
count=$(echo "$result" | grep -c "sensor_id" || true)
if [ "$count" -eq 2 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 lines with non-empty sensor_id, got $count"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 9: Output format -if json (explicit)
echo ""
echo "Test 9: Output format -if json (explicit)"
result=$(echo '{"sensor":"ds18b20","value":"22.5"}' | ./sensor-data transform -if json)
if echo "$result" | grep -q '"sensor"' && echo "$result" | grep -q '"value"'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 10: Multi-object JSON line passthrough
echo ""
echo "Test 10: Multi-object JSON line passthrough"
input='[ { "sensor": "ds18b20", "value": 22.5 }, { "sensor": "dht22", "value": 45 } ]'
result=$(echo "$input" | ./sensor-data transform)
if [ "$result" = "$input" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: $input"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 11: Multiple input lines streaming
echo ""
echo "Test 11: Multiple input lines streaming"
result=$(cat <<'EOF' | ./sensor-data transform
[ { "sensor": "ds18b20", "value": 22.5 } ]
[ { "sensor": "dht22", "value": 45 } ]
[ { "sensor": "bmp280", "value": 1013 } ]
EOF
)
count=$(echo "$result" | wc -l | tr -d ' ')
if [ "$count" -eq 3 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 3 output lines, got $count"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 11a: Multi-object JSON array with sensor filter
echo ""
echo "Test 11a: Multi-object JSON array with sensor filter"
result=$(echo '[ { "sensor": "ds18b20", "value": 22.5 }, { "sensor": "dht22", "value": 45 }, { "sensor": "ds18b20", "value": 23.0 } ]' | ./sensor-data transform --only-value sensor:ds18b20)
# Should filter out dht22, keep both ds18b20 readings
if echo "$result" | grep -q 'ds18b20' && ! echo "$result" | grep -q 'dht22'; then
    count=$(echo "$result" | grep -o 'ds18b20' | wc -l | tr -d ' ')
    if [ "$count" -eq 2 ]; then
        echo "  ✓ PASS"
        PASSED=$((PASSED + 1))
    else
        echo "  ✗ FAIL - Expected 2 ds18b20 readings, got $count"
        FAILED=$((FAILED + 1))
    fi
else
    echo "  ✗ FAIL - Expected only ds18b20 sensors"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 11b: Multi-object JSON array with --remove-errors
echo ""
echo "Test 11b: Multi-object JSON array with --remove-errors"
result=$(echo '[ { "sensor": "ds18b20", "value": "85" }, { "sensor": "ds18b20", "value": "22.5" }, { "sensor": "ds18b20", "value": "-127" } ]' | ./sensor-data transform --remove-errors)
# Should filter out error values 85 and -127, keep only 22.5
if echo "$result" | grep -q "22.5" && ! echo "$result" | grep -q ":85" && ! echo "$result" | grep -q ":-127"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected only valid reading (22.5)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 11c: Multi-object JSON array with --only-value filter
echo ""
echo "Test 11c: Multi-object JSON array with --only-value filter"
result=$(echo '[ { "sensor": "ds18b20", "value": "22.5" }, { "sensor": "ds18b20", "value": "23.0" }, { "sensor": "ds18b20", "value": "22.5" } ]' | ./sensor-data transform --only-value value:22.5)
# Should keep only readings with value=22.5
count=$(echo "$result" | grep -o "22.5" | wc -l | tr -d ' ')
if [ "$count" -eq 2 ] && ! echo "$result" | grep -q "23.0"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 readings with value=22.5"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 11d: Multi-object JSON array where all objects filtered out
echo ""
echo "Test 11d: Multi-object JSON array where all objects filtered out"
result=$(echo '[ { "sensor": "dht22", "value": "45" }, { "sensor": "bmp280", "value": "1013" } ]' | ./sensor-data transform --only-value sensor:ds18b20)
# Should produce empty output when all objects are filtered
if [ -z "$result" ] || [ "$result" = "[]" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected empty output"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 11e: Multiple multi-object JSON lines with filter
echo ""
echo "Test 11e: Multiple multi-object JSON lines with filter"
result=$(cat <<'EOF' | ./sensor-data transform --only-value sensor:ds18b20
[ { "sensor": "ds18b20", "value": "22.5" }, { "sensor": "dht22", "value": "45" } ]
[ { "sensor": "bmp280", "value": "1013" }, { "sensor": "ds18b20", "value": "23.0" } ]
[ { "sensor": "dht22", "value": "50" } ]
EOF
)
# Should filter to 2 output lines (one from each line that has ds18b20), third line fully filtered
count=$(echo "$result" | grep -c "ds18b20" || true)
if [ "$count" -eq 2 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 lines with ds18b20, got $count"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 12: CSV input to JSON output
echo ""
echo "Test 12: CSV input to JSON output"
result=$(cat <<'EOF' | ./sensor-data transform -if csv -of json
sensor,value
ds18b20,22.5
dht22,45
EOF
)
if echo "$result" | grep -q '"sensor".*"ds18b20"' && echo "$result" | grep -q '"sensor".*"dht22"'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 13: Empty input produces no output
echo ""
echo "Test 13: Empty input produces no output"
result=$(echo "" | ./sensor-data transform)
if [ -z "$result" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected empty output"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 14: Directory traversal (non-recursive)
echo ""
echo "Test 14: Directory traversal (non-recursive)"
mkdir -p testdir
echo '{"sensor":"ds18b20","value":"20.0"}' > testdir/file1.out
echo '{"sensor":"ds18b20","value":"21.0"}' > testdir/file2.out
echo '{"sensor":"ds18b20","value":"22.0"}' > testdir/file3.txt
./sensor-data transform -if csv -e .out testdir/ -o output.csv
count=$(grep -c "ds18b20" output.csv || true)
rm -rf testdir output.csv
if [ "$count" -eq 2 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 rows, got $count"
    FAILED=$((FAILED + 1))
fi

# Test 15: Directory traversal (recursive)
echo ""
echo "Test 15: Directory traversal (recursive)"
mkdir -p testdir/subdir1/subdir2
echo '{"sensor":"ds18b20","value":"20.0"}' > testdir/file1.out
echo '{"sensor":"ds18b20","value":"21.0"}' > testdir/subdir1/file2.out
echo '{"sensor":"ds18b20","value":"22.0"}' > testdir/subdir1/subdir2/file3.out
./sensor-data transform -if csv -r -e .out testdir/ -o output.csv
count=$(grep -c "ds18b20" output.csv || true)
rm -rf testdir output.csv
if [ "$count" -eq 3 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 3 rows, got $count"
    FAILED=$((FAILED + 1))
fi

# Test 16: Depth 0 (root only)
echo ""
echo "Test 16: Depth 0 (root only)"
mkdir -p testdir/subdir1
echo '{"sensor":"ds18b20","value":"20.0"}' > testdir/file1.out
echo '{"sensor":"ds18b20","value":"21.0"}' > testdir/subdir1/file2.out
./sensor-data transform -if csv -r -d 0 -e .out testdir/ -o output.csv
count=$(grep -c "ds18b20" output.csv || true)
rm -rf testdir output.csv
if [ "$count" -eq 1 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 1 row, got $count"
    FAILED=$((FAILED + 1))
fi

# Test 17: Depth 1
echo ""
echo "Test 17: Depth 1"
mkdir -p testdir/subdir1/subdir2
echo '{"sensor":"ds18b20","value":"20.0"}' > testdir/file1.out
echo '{"sensor":"ds18b20","value":"21.0"}' > testdir/subdir1/file2.out
echo '{"sensor":"ds18b20","value":"22.0"}' > testdir/subdir1/subdir2/file3.out
./sensor-data transform -if csv -r -d 1 -e .out testdir/ -o output.csv
count=$(grep -c "ds18b20" output.csv || true)
rm -rf testdir output.csv
if [ "$count" -eq 2 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 rows, got $count"
    FAILED=$((FAILED + 1))
fi

# Test 18: Depth 2 (three levels)
echo ""
echo "Test 18: Depth 2 (three levels)"
mkdir -p testdir/sub1/sub2/sub3
echo '{"sensor":"ds18b20","value":"1"}' > testdir/file.out
echo '{"sensor":"ds18b20","value":"2"}' > testdir/sub1/file.out
echo '{"sensor":"ds18b20","value":"3"}' > testdir/sub1/sub2/file.out
echo '{"sensor":"ds18b20","value":"4"}' > testdir/sub1/sub2/sub3/file.out
./sensor-data transform -if csv -r -d 2 -e .out testdir/ -o output.csv
count=$(grep -c "ds18b20" output.csv || true)
rm -rf testdir output.csv
if [ "$count" -eq 3 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 3 rows, got $count"
    FAILED=$((FAILED + 1))
fi

# Test 19: Unlimited depth
echo ""
echo "Test 19: Unlimited depth"
mkdir -p testdir/a/b/c/d
echo '{"sensor":"ds18b20","value":"1"}' > testdir/file.out
echo '{"sensor":"ds18b20","value":"2"}' > testdir/a/file.out
echo '{"sensor":"ds18b20","value":"3"}' > testdir/a/b/file.out
echo '{"sensor":"ds18b20","value":"4"}' > testdir/a/b/c/file.out
echo '{"sensor":"ds18b20","value":"5"}' > testdir/a/b/c/d/file.out
./sensor-data transform -if csv -r -e .out testdir/ -o output.csv
count=$(grep -c "ds18b20" output.csv || true)
rm -rf testdir output.csv
if [ "$count" -eq 5 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 5 rows, got $count"
    FAILED=$((FAILED + 1))
fi

# Test 20: Verbose output (-v)
echo ""
echo "Test 20: Verbose output (-v)"
mkdir -p testdir
echo '{"sensor":"ds18b20","value":"20.0"}' > testdir/file1.out
output=$(./sensor-data transform -v -e .out testdir/ -o output.csv 2>&1)
rm -rf testdir output.csv
if echo "$output" | grep -q "Scanning"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 'Scanning' in output"
    FAILED=$((FAILED + 1))
fi

# Test 21: Very verbose output (-V)
echo ""
echo "Test 21: Very verbose output (-V)"
mkdir -p testdir
echo '{"sensor":"ds18b20","value":"20.0"}' > testdir/file1.out
echo '{"sensor":"ds18b20","value":"21.0"}' > testdir/file2.txt
output=$(./sensor-data transform -V -e .out testdir/ -o output.csv 2>&1)
rm -rf testdir output.csv
if echo "$output" | grep -q "Found file\|Skipping"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 'Found file' or 'Skipping' in output"
    FAILED=$((FAILED + 1))
fi

# Test 22: Depth limit skip message (-V -d 0)
echo ""
echo "Test 22: Depth limit skip message (-V -d 0)"
mkdir -p testdir/subdir1/subdir2
echo '{"sensor":"ds18b20","value":"20.0"}' > testdir/file1.out
echo '{"sensor":"ds18b20","value":"21.0"}' > testdir/subdir1/file2.out
output=$(./sensor-data transform -r -d 0 -V -e .out testdir/ -o output.csv 2>&1)
rm -rf testdir output.csv
if echo "$output" | grep -q "Skipping directory (depth limit)"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 'Skipping directory (depth limit)' in output"
    FAILED=$((FAILED + 1))
fi

# Test 22a: --use-prototype uses sc-prototype for column definitions
echo ""
echo "Test 22a: --use-prototype uses sc-prototype for column definitions"
# Create test data
mkdir -p testdir
echo '{"timestamp": 1234567890, "sensor": "ds18b20", "value": 22.5}' > testdir/test.out
# Run with --use-prototype (sc-prototype should be available from sensor-control package)
output=$(./sensor-data transform --use-prototype testdir/test.out 2>&1)
rm -rf testdir
# Check that it mentions loading columns from sc-prototype
if echo "$output" | grep -q "sc-prototype\|columns"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected sc-prototype to be used for column definitions"
    echo "  Got: $output"
    FAILED=$((FAILED + 1))
fi

# Test 22b: --use-prototype with CSV output
echo ""
echo "Test 22b: --use-prototype produces valid CSV output"
mkdir -p testdir
echo '{"timestamp": 1234567890, "sensor": "ds18b20", "value": 22.5}' > testdir/test.out
output=$(./sensor-data transform --use-prototype -if csv testdir/test.out 2>&1)
rm -rf testdir
# Check that output contains data rows (not just errors)
if echo "$output" | grep -q "22.5\|ds18b20\|1234567890"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected CSV output with sensor data"
    echo "  Got: $output"
    FAILED=$((FAILED + 1))
fi

# Test 22c: --use-prototype with file output
echo ""
echo "Test 22c: --use-prototype with file output"
mkdir -p testdir
echo '{"timestamp": 1234567890, "sensor": "ds18b20", "value": 22.5}' > testdir/test.out
./sensor-data transform --use-prototype -if csv -o output.csv testdir/test.out 2>/dev/null
if [ -f output.csv ] && grep -q "22.5\|ds18b20" output.csv; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected output.csv with sensor data"
    if [ -f output.csv ]; then
        echo "  File contents: $(cat output.csv)"
    else
        echo "  File not created"
    fi
    FAILED=$((FAILED + 1))
fi
rm -rf testdir output.csv

# Test 23: JSON file to JSON output with filtering (exercises writeRowsFromFileJson filter path)
echo ""
echo "Test 23: JSON file to JSON with --only-value filter"
mkdir -p testdir
cat > testdir/test.out << 'EOF'
[{"sensor": "ds18b20", "value": "22.5"}, {"sensor": "dht22", "value": "45"}]
[{"sensor": "bmp280", "value": "1013"}, {"sensor": "ds18b20", "value": "23.0"}]
EOF
result=$(./sensor-data transform -if json --only-value sensor:ds18b20 testdir/test.out)
rm -rf testdir
# Should filter to only ds18b20 readings (2 readings from 2 lines)
count=$(echo "$result" | grep -c "ds18b20" || true)
if [ "$count" -eq 2 ] && ! echo "$result" | grep -q "dht22\|bmp280"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 ds18b20 readings only"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 23a: JSON file to JSON with --remove-errors filter
echo ""
echo "Test 23a: JSON file to JSON with --remove-errors filter"
mkdir -p testdir
cat > testdir/test.out << 'EOF'
[{"sensor": "ds18b20", "value": "85"}, {"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "ds18b20", "value": "-127"}, {"sensor": "ds18b20", "value": "23.0"}]
EOF
result=$(./sensor-data transform -if json --remove-errors testdir/test.out)
rm -rf testdir
# Should remove error values 85 and -127, keep 22.5 and 23.0
if echo "$result" | grep -q "22.5" && echo "$result" | grep -q "23.0" && ! echo "$result" | grep -q '":85"\|":-127"'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected only valid readings (22.5, 23.0)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 23b: JSON file to JSON with date filter
echo ""
echo "Test 23b: JSON file to JSON with date filter"
mkdir -p testdir
cat > testdir/test.out << 'EOF'
[{"timestamp": "1704067200", "sensor": "ds18b20", "value": "22.5"}]
[{"timestamp": "1706745600", "sensor": "ds18b20", "value": "23.0"}]
[{"timestamp": "1709424000", "sensor": "ds18b20", "value": "24.0"}]
EOF
# Filter to readings in January 2024 (1704067200 = 2024-01-01)
result=$(./sensor-data transform -if json --min-date 2024-01-01 --max-date 2024-01-31 testdir/test.out)
rm -rf testdir
# Should only include the first reading (Jan 2024)
if echo "$result" | grep -q "22.5" && ! echo "$result" | grep -q "23.0\|24.0"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected only January 2024 reading"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 23c: JSON file to JSON where entire line is filtered out
echo ""
echo "Test 23c: JSON file to JSON with all readings filtered from some lines"
mkdir -p testdir
cat > testdir/test.out << 'EOF'
[{"sensor": "dht22", "value": "45"}, {"sensor": "bmp280", "value": "1013"}]
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "dht22", "value": "50"}]
EOF
result=$(./sensor-data transform -if json --only-value sensor:ds18b20 testdir/test.out)
rm -rf testdir
# Should output only line with ds18b20 (other lines completely filtered)
line_count=$(echo "$result" | grep -c '\[' || true)
if [ "$line_count" -eq 1 ] && echo "$result" | grep -q "ds18b20"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 1 output line with ds18b20"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 23d: JSON file to JSON with multiple readings filtered to one per line
echo ""
echo "Test 23d: JSON file with multiple objects, some filtered per line"
mkdir -p testdir
cat > testdir/test.out << 'EOF'
[{"sensor": "ds18b20", "value": "22.5"}, {"sensor": "ds18b20", "value": "85"}, {"sensor": "dht22", "value": "45"}]
EOF
result=$(./sensor-data transform -if json --remove-errors --only-value sensor:ds18b20 testdir/test.out)
rm -rf testdir
# Should keep only non-error ds18b20 reading (22.5), filter out 85 (error) and dht22
if echo "$result" | grep -q "22.5" && ! echo "$result" | grep -q '":85"\|dht22'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected only valid ds18b20 reading (22.5)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 23e: JSON file to JSON output to file with filtering
echo ""
echo "Test 23e: JSON file to JSON file output with filtering"
mkdir -p testdir
cat > testdir/test.out << 'EOF'
[{"sensor": "ds18b20", "value": "22.5"}, {"sensor": "dht22", "value": "45"}]
EOF
./sensor-data transform -if json --only-value sensor:ds18b20 -o output.json testdir/test.out
if [ -f output.json ] && grep -q "ds18b20" output.json && ! grep -q "dht22" output.json; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected output.json with only ds18b20"
    if [ -f output.json ]; then
        echo "  File contents: $(cat output.json)"
    else
        echo "  File not created"
    fi
    FAILED=$((FAILED + 1))
fi
rm -rf testdir output.json

# Test 24: CSV stdin to JSON output with filtering (exercises processStdinDataJson filter path)
echo ""
echo "Test 24: CSV stdin to JSON with --only-value filter"
result=$(cat <<'EOF' | ./sensor-data transform -if csv -of json --only-value sensor:ds18b20
sensor,value
ds18b20,22.5
dht22,45
ds18b20,23.0
EOF
)
# Should filter to only ds18b20 readings
if echo "$result" | grep -q "ds18b20" && ! echo "$result" | grep -q "dht22"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected only ds18b20 readings"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 24a: CSV stdin to JSON with --remove-errors filter
echo ""
echo "Test 24a: CSV stdin to JSON with --remove-errors filter"
result=$(cat <<'EOF' | ./sensor-data transform -if csv -of json --remove-errors
sensor,value
ds18b20,85
ds18b20,22.5
ds18b20,-127
ds18b20,23.0
EOF
)
# Should remove error values 85 and -127, keep 22.5 and 23.0
if echo "$result" | grep -q "22.5" && echo "$result" | grep -q "23.0" && ! echo "$result" | grep -q '":85"\|":-127"'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected only valid readings (22.5, 23.0)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 24b: CSV stdin to JSON output to file with filtering
echo ""
echo "Test 24b: CSV stdin to JSON file with --only-value filter"
cat <<'EOF' | ./sensor-data transform -if csv -of json --only-value sensor:ds18b20 -o output.json
sensor,value
ds18b20,22.5
dht22,45
EOF
if [ -f output.json ] && grep -q "ds18b20" output.json && ! grep -q "dht22" output.json; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected output.json with only ds18b20"
    if [ -f output.json ]; then
        echo "  File contents: $(cat output.json)"
    else
        echo "  File not created"
    fi
    FAILED=$((FAILED + 1))
fi
rm -f output.json

# Test 25: --exclude-value filter (exclude specific sensor)
echo ""
echo "Test 25: --exclude-value filter"
result=$(cat <<'EOF' | ./sensor-data transform --exclude-value sensor:dht22
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "dht22", "value": "45"}]
[{"sensor": "ds18b20", "value": "23.0"}]
EOF
)
# Should exclude dht22 readings, keep ds18b20
if echo "$result" | grep -q "ds18b20" && ! echo "$result" | grep -q "dht22"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected ds18b20 readings only (dht22 excluded)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 25a: --exclude-value with multiple exclusions
echo ""
echo "Test 25a: --exclude-value with multiple exclusions"
result=$(cat <<'EOF' | ./sensor-data transform --exclude-value sensor:dht22 --exclude-value sensor:bmp280
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "dht22", "value": "45"}]
[{"sensor": "bmp280", "value": "1013"}]
[{"sensor": "ds18b20", "value": "23.0"}]
EOF
)
# Should exclude both dht22 and bmp280
if echo "$result" | grep -q "ds18b20" && ! echo "$result" | grep -q "dht22\|bmp280"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected only ds18b20 (dht22 and bmp280 excluded)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 25b: --exclude-value combined with --only-value
echo ""
echo "Test 25b: --exclude-value combined with --only-value"
result=$(cat <<'EOF' | ./sensor-data transform --only-value sensor:ds18b20 --exclude-value value:85
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "ds18b20", "value": "85"}]
[{"sensor": "dht22", "value": "45"}]
[{"sensor": "ds18b20", "value": "23.0"}]
EOF
)
# Should include only ds18b20 but exclude the one with value=85
count=$(echo "$result" | grep -c "ds18b20" || true)
if [ "$count" -eq 2 ] && ! echo "$result" | grep -q '"value": "85"'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 ds18b20 readings (excluding value=85)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 25c: --exclude-value on CSV output
echo ""
echo "Test 25c: --exclude-value with CSV output"
result=$(cat <<'EOF' | ./sensor-data transform -of csv --exclude-value sensor:dht22
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "dht22", "value": "45"}]
[{"sensor": "ds18b20", "value": "23.0"}]
EOF
)
# Should have CSV with ds18b20 rows only
ds_count=$(echo "$result" | grep -c "ds18b20" || true)
dht_count=$(echo "$result" | grep -c "dht22" || true)
if [ "$ds_count" -eq 2 ] && [ "$dht_count" -eq 0 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 ds18b20 rows in CSV, 0 dht22"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 25d: --exclude-value with file input
echo ""
echo "Test 25d: --exclude-value with file input"
mkdir -p testdir
cat > testdir/test.out << 'EOF'
[{"sensor": "ds18b20", "value": "22.5"}, {"sensor": "dht22", "value": "45"}]
[{"sensor": "bmp280", "value": "1013"}, {"sensor": "ds18b20", "value": "23.0"}]
EOF
result=$(./sensor-data transform -if json --exclude-value sensor:dht22 --exclude-value sensor:bmp280 testdir/test.out)
rm -rf testdir
# Should exclude dht22 and bmp280, keep only ds18b20
if echo "$result" | grep -q "ds18b20" && ! echo "$result" | grep -q "dht22\|bmp280"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected only ds18b20 readings"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 26: Verbose output with --not-empty filter
echo ""
echo "Test 26: Verbose output with --not-empty filter"
result=$(echo '{"sensor": "ds18b20", "value": "22.5"}' | ./sensor-data transform -v --not-empty sensor 2>&1)
if echo "$result" | grep -q "Required non-empty columns:.*sensor"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected verbose output showing required non-empty columns"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 26a: Verbose output with --only-value filter
echo ""
echo "Test 26a: Verbose output with --only-value filter"
result=$(echo '{"sensor": "ds18b20", "value": "22.5"}' | ./sensor-data transform -v --only-value sensor:ds18b20 2>&1)
if echo "$result" | grep -q "Value filters (include):.*sensor=ds18b20"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected verbose output showing include value filters"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 26b: Verbose output with --exclude-value filter
echo ""
echo "Test 26b: Verbose output with --exclude-value filter"
result=$(echo '{"sensor": "ds18b20", "value": "22.5"}' | ./sensor-data transform -v --exclude-value sensor:dht22 2>&1)
if echo "$result" | grep -q "Value filters (exclude):.*sensor=dht22"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected verbose output showing exclude value filters"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 26c: Verbose output with multiple filters combined
echo ""
echo "Test 26c: Verbose output with multiple filters combined"
result=$(echo '{"sensor": "ds18b20", "value": "22.5"}' | ./sensor-data transform -v --not-empty value --only-value sensor:ds18b20 --exclude-value value:85 2>&1)
if echo "$result" | grep -q "Required non-empty columns:" && \
   echo "$result" | grep -q "Value filters (include):" && \
   echo "$result" | grep -q "Value filters (exclude):"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected verbose output showing all filter types"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 26d: Verbose output with file input and --not-empty filter
echo ""
echo "Test 26d: Verbose file input with --not-empty filter"
mkdir -p testdir
echo '[{"sensor": "ds18b20", "value": "22.5"}]' > testdir/verbose_test.out
result=$(./sensor-data transform -v --not-empty sensor testdir/verbose_test.out 2>&1)
if echo "$result" | grep -q "Required non-empty columns:.*sensor"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected verbose output showing required non-empty columns"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 26e: Verbose output with file input and --only-value filter
echo ""
echo "Test 26e: Verbose file input with --only-value filter"
result=$(./sensor-data transform -v --only-value sensor:ds18b20 testdir/verbose_test.out 2>&1)
if echo "$result" | grep -q "Value filters (include):.*sensor=ds18b20"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected verbose output showing include value filters"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 26f: Verbose output with file input and --exclude-value filter
echo ""
echo "Test 26f: Verbose file input with --exclude-value filter"
result=$(./sensor-data transform -v --exclude-value sensor:dht22 testdir/verbose_test.out 2>&1)
if echo "$result" | grep -q "Value filters (exclude):.*sensor=dht22"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected verbose output showing exclude value filters"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 26g: Verbose output with file input and multiple filters combined
echo ""
echo "Test 26g: Verbose file input with multiple filters combined"
result=$(./sensor-data transform -v --not-empty value --only-value sensor:ds18b20 --exclude-value value:85 testdir/verbose_test.out 2>&1)
rm -rf testdir
if echo "$result" | grep -q "Required non-empty columns:" && \
   echo "$result" | grep -q "Value filters (include):" && \
   echo "$result" | grep -q "Value filters (exclude):"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected verbose output showing all filter types"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 27: list-rejects command outputs rejected readings
echo ""
echo "Test 27: list-rejects outputs rejected readings"
input='[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "ds18b20", "value": "85"}]
[{"sensor": "ds18b20", "value": "23.0"}]'
result=$(echo "$input" | ./sensor-data list-rejects --remove-errors)
# Should output only the error reading (value=85)
if echo "$result" | grep -q '"value": "85"\|"value": 85' && ! echo "$result" | grep -q '"value": "22.5"\|"value": 22\.5'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected only error reading (value=85)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 28: list-rejects with --not-empty shows empty value rows
echo ""
echo "Test 28: list-rejects --not-empty shows rows with empty values"
input='[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "ds18b20", "value": ""}]
[{"sensor": "ds18b20", "value": "23.0"}]'
result=$(echo "$input" | ./sensor-data list-rejects --not-empty value)
# Should output only the row with empty value (could be "" or null)
if echo "$result" | grep -qE '"value": (""|null)' && ! echo "$result" | grep -q '"value": "22.5"\|"value": 22\.5'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected only row with empty value"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 29: list-rejects --help shows list-rejects usage
echo ""
echo "Test 29: list-rejects --help shows usage"
result=$(./sensor-data list-rejects --help 2>&1) || true
if echo "$result" | grep -q "list-rejects"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected list-rejects usage text"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Summary
echo ""
echo "================================"
echo "transform Tests Summary"
echo "================================"
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo ""

if [ $FAILED -gt 0 ]; then
    exit 1
fi

echo "All transform tests passed!"
