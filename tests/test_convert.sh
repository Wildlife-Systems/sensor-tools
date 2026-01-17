#!/bin/bash
# Integration tests for convert command with various options

set -e  # Exit on error

echo "================================"
echo "Testing convert functionality"
echo "================================"

# Build sensor-data if not already built
if [ ! -f sensor-data ]; then
    echo "Building sensor-data..."
    make sensor-data
fi

PASSED=0
FAILED=0

# Test 1: JSON passthrough (no filtering)
echo "Test 1: JSON passthrough (no filtering)"
input='[ { "sensor": "ds18b20", "value": 22.5 } ]'
result=$(echo "$input" | ./sensor-data convert)
if [ "$result" = "$input" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: $input"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 2: JSON to CSV conversion
echo ""
echo "Test 2: JSON to CSV conversion"
result=$(echo '{"sensor":"ds18b20","value":"22.5"}' | ./sensor-data convert -F csv)
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
result=$(cat <<'EOF' | ./sensor-data convert --min-date 2026-01-15
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
result=$(cat <<'EOF' | ./sensor-data convert --max-date 2026-01-15
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
result=$(cat <<'EOF' | ./sensor-data convert --min-date 2026-01-12 --max-date 2026-01-18
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
result=$(cat <<'EOF' | ./sensor-data convert --remove-errors
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
result=$(cat <<'EOF' | ./sensor-data convert --only-value value=22.5
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
result=$(cat <<'EOF' | ./sensor-data convert --not-empty sensor_id
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

# Test 9: Output format -F json (explicit)
echo ""
echo "Test 9: Output format -F json (explicit)"
result=$(echo '{"sensor":"ds18b20","value":"22.5"}' | ./sensor-data convert -F json)
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
result=$(echo "$input" | ./sensor-data convert)
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
result=$(cat <<'EOF' | ./sensor-data convert
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

# Test 12: CSV input to JSON output
echo ""
echo "Test 12: CSV input to JSON output"
result=$(cat <<'EOF' | ./sensor-data convert -f csv -F json
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
result=$(echo "" | ./sensor-data convert)
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
./sensor-data convert -F csv -e .out testdir/ -o output.csv
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
./sensor-data convert -F csv -r -e .out testdir/ -o output.csv
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
./sensor-data convert -F csv -r -d 0 -e .out testdir/ -o output.csv
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
./sensor-data convert -F csv -r -d 1 -e .out testdir/ -o output.csv
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
./sensor-data convert -F csv -r -d 2 -e .out testdir/ -o output.csv
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
./sensor-data convert -F csv -r -e .out testdir/ -o output.csv
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
output=$(./sensor-data convert -v -e .out testdir/ -o output.csv 2>&1)
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
output=$(./sensor-data convert -V -e .out testdir/ -o output.csv 2>&1)
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
output=$(./sensor-data convert -r -d 0 -V -e .out testdir/ -o output.csv 2>&1)
rm -rf testdir output.csv
if echo "$output" | grep -q "Skipping directory (depth limit)"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 'Skipping directory (depth limit)' in output"
    FAILED=$((FAILED + 1))
fi

# Summary
echo ""
echo "================================"
echo "Convert Tests Summary"
echo "================================"
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo ""

if [ $FAILED -gt 0 ]; then
    exit 1
fi

echo "All convert tests passed!"
