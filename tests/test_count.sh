#!/bin/bash
# Integration tests for count command

set -e  # Exit on error

# Use gtimeout on macOS (from coreutils), timeout on Linux
if command -v gtimeout &> /dev/null; then
    TIMEOUT_CMD="gtimeout"
elif command -v timeout &> /dev/null; then
    TIMEOUT_CMD="timeout"
else
    TIMEOUT_CMD=""
fi

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
result=$(echo "$input" | ./sensor-data count -if csv)
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
result=$(echo "$input" | ./sensor-data count -if csv --only-value type:ds18b20)
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

# ================================
# FILE-BASED TESTS
# ================================

echo ""
echo "================================"
echo "File-based count tests"
echo "================================"

# Create temporary test files
TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

# Test: Count from JSON file
echo ""
echo "Test: Count from JSON file"
cat > "$TMPDIR/test.out" << 'EOF'
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "ds18b20", "value": "23.0"}]
[{"sensor": "bme280", "value": "24.0"}]
EOF
result=$(./sensor-data count "$TMPDIR/test.out")
if [ "$result" = "3" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 3"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count from JSON file with filter
echo ""
echo "Test: Count from JSON file with --only-value filter"
result=$(./sensor-data count --only-value sensor:ds18b20 "$TMPDIR/test.out")
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count from CSV file
echo ""
echo "Test: Count from CSV file"
cat > "$TMPDIR/test.csv" << 'EOF'
sensor,value,unit
ds18b20,22.5,C
ds18b20,23.0,C
bme280,24.0,C
dht22,25.0,C
EOF
result=$(./sensor-data count "$TMPDIR/test.csv")
if [ "$result" = "4" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 4"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count from CSV file with filter
echo ""
echo "Test: Count from CSV file with --only-value filter"
result=$(./sensor-data count --only-value sensor:ds18b20 "$TMPDIR/test.csv")
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count from CSV file with --remove-errors
echo ""
echo "Test: Count from CSV file with --remove-errors"
cat > "$TMPDIR/test-errors.csv" << 'EOF'
sensor,value
ds18b20,22.5
ds18b20,85
ds18b20,-127
ds18b20,23.0
EOF
result=$(./sensor-data count --remove-errors "$TMPDIR/test-errors.csv")
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count from JSON file with --remove-errors
echo ""
echo "Test: Count from JSON file with --remove-errors"
cat > "$TMPDIR/test-errors.out" << 'EOF'
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "ds18b20", "value": "85"}]
[{"sensor": "ds18b20", "value": "-127"}]
[{"sensor": "ds18b20", "value": "23.0"}]
EOF
result=$(./sensor-data count --remove-errors "$TMPDIR/test-errors.out")
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count from JSON file with --remove-empty-json
echo ""
echo "Test: Count from JSON file with --remove-empty-json"
cat > "$TMPDIR/test-empty.out" << 'EOF'
[{}]
[]
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "ds18b20", "value": "23.0"}]
EOF
result=$(./sensor-data count --remove-empty-json "$TMPDIR/test-empty.out")
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count multiple files
echo ""
echo "Test: Count from multiple files"
cat > "$TMPDIR/file1.out" << 'EOF'
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "ds18b20", "value": "23.0"}]
EOF
cat > "$TMPDIR/file2.out" << 'EOF'
[{"sensor": "bme280", "value": "24.0"}]
[{"sensor": "dht22", "value": "25.0"}]
[{"sensor": "ds18b20", "value": "26.0"}]
EOF
result=$(./sensor-data count "$TMPDIR/file1.out" "$TMPDIR/file2.out")
if [ "$result" = "5" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 5"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count with --not-empty from file
echo ""
echo "Test: Count from file with --not-empty filter"
cat > "$TMPDIR/test-empty-values.out" << 'EOF'
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "ds18b20", "value": ""}]
[{"sensor": "ds18b20"}]
[{"sensor": "ds18b20", "value": "23.0"}]
EOF
result=$(./sensor-data count --not-empty value "$TMPDIR/test-empty-values.out")
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Count with --clean (combines --remove-empty-json --not-empty value --remove-errors)
echo ""
echo "Test: Count with --clean filter"
result=$(cat <<'EOF' | ./sensor-data count --clean
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "ds18b20", "value": ""}]
[{"sensor": "ds18b20", "value": "85"}]
[{"sensor": "ds18b20", "value": "23.0"}]
[{"sensor": "ds18b20", "value": "-127"}]
[{}]
EOF
)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2 (--clean excludes empty values, errors, and empty JSON)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --allowed-values with comma-separated list
echo ""
echo "Test: --allowed-values with comma-separated values"
result=$(cat <<'EOF' | ./sensor-data count --allowed-values sensor ds18b20,dht22
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "dht22", "value": "45"}]
[{"sensor": "bmp280", "value": "1013"}]
[{"sensor": "ds18b20", "value": "23.0"}]
EOF
)
if [ "$result" = "3" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 3 (ds18b20 x2 + dht22 x1)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --allowed-values with file
echo ""
echo "Test: --allowed-values with file of values"
# Create a temp file with allowed values
tmpfile=$(mktemp)
echo "ds18b20" > "$tmpfile"
echo "dht22" >> "$tmpfile"
result=$(cat <<'EOF' | ./sensor-data count --allowed-values sensor "$tmpfile"
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "dht22", "value": "45"}]
[{"sensor": "bmp280", "value": "1013"}]
[{"sensor": "ds18b20", "value": "23.0"}]
EOF
)
rm -f "$tmpfile"
if [ "$result" = "3" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 3 (ds18b20 x2 + dht22 x1)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --allowed-values with single value
echo ""
echo "Test: --allowed-values with single value"
result=$(cat <<'EOF' | ./sensor-data count --allowed-values sensor ds18b20
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "dht22", "value": "45"}]
[{"sensor": "bmp280", "value": "1013"}]
[{"sensor": "ds18b20", "value": "23.0"}]
EOF
)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2 (ds18b20 x2)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --by-column with human output (default)
echo ""
echo "Test: --by-column with human output"
result=$(cat <<'EOF' | ./sensor-data count --by-column sensor
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "dht22", "value": "45"}]
[{"sensor": "ds18b20", "value": "23.0"}]
EOF
)
if echo "$result" | grep -q "Counts by sensor" && echo "$result" | grep -q "ds18b20" && echo "$result" | grep -q "dht22" && echo "$result" | grep -q "Total: 3"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: Human-readable counts by sensor"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --by-column with csv output
echo ""
echo "Test: --by-column with csv output"
result=$(cat <<'EOF' | ./sensor-data count -b sensor -of csv
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "dht22", "value": "45"}]
[{"sensor": "ds18b20", "value": "23.0"}]
EOF
)
if echo "$result" | head -1 | grep -q "sensor,count" && echo "$result" | grep -q "ds18b20,2" && echo "$result" | grep -q "dht22,1"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: CSV with sensor,count header and data rows"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --by-column with json output
echo ""
echo "Test: --by-column with json output"
result=$(cat <<'EOF' | ./sensor-data count -b sensor -of json
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "dht22", "value": "45"}]
[{"sensor": "ds18b20", "value": "23.0"}]
EOF
)
# Sorted by count descending: ds18b20 (2) before dht22 (1)
if echo "$result" | grep -q '\[{"sensor":"ds18b20","count":2},{"sensor":"dht22","count":1}\]'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: JSON array with sensor/count objects (sorted by count descending)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --by-column with filters
echo ""
echo "Test: --by-column with --remove-errors filter"
result=$(cat <<'EOF' | ./sensor-data count -b sensor --remove-errors -of csv
[{"sensor": "ds18b20", "value": "22.5"}]
[{"sensor": "ds18b20", "value": "85"}]
[{"sensor": "dht22", "value": "45"}]
EOF
)
if echo "$result" | grep -q "ds18b20,1" && echo "$result" | grep -q "dht22,1"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: ds18b20,1 and dht22,1 (error reading excluded)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --by-column with non-existent column
echo ""
echo "Test: --by-column with non-existent column returns empty values"
result=$(cat <<'EOF' | ./sensor-data count -b missing_column -of csv
[{"sensor": "ds18b20", "value": "22.5"}]
EOF
)
if echo "$result" | grep -q "missing_column,count" && echo "$result" | grep -q ",1"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: Count with empty column value"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --not-null filters out "null" string values
echo ""
echo "Test: --not-null filters out literal 'null' string"
result=$(cat <<'EOF' | ./sensor-data count --not-null value
[{"sensor_id": "s1", "value": "10"}]
[{"sensor_id": "s2", "value": "null"}]
[{"sensor_id": "s3", "value": "20"}]
EOF
)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --not-null on sensor_id column
echo ""
echo "Test: --not-null filters null sensor_id"
result=$(cat <<'EOF' | ./sensor-data count --not-null sensor_id
[{"sensor_id": "s1", "value": "10"}]
[{"sensor_id": "null", "value": "20"}]
[{"sensor_id": "s3", "value": "30"}]
EOF
)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --not-null can be used multiple times
echo ""
echo "Test: --not-null can filter multiple columns"
result=$(cat <<'EOF' | ./sensor-data count --not-null value --not-null sensor_id
[{"sensor_id": "s1", "value": "10"}]
[{"sensor_id": "null", "value": "20"}]
[{"sensor_id": "s3", "value": "null"}]
[{"sensor_id": "s4", "value": "40"}]
EOF
)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --clean now includes --not-null for value and sensor_id
echo ""
echo "Test: --clean filters null values in value and sensor_id"
result=$(cat <<'EOF' | ./sensor-data count --clean
[{"sensor_id": "s1", "value": "10"}]
[{"sensor_id": "null", "value": "20"}]
[{"sensor_id": "s3", "value": "null"}]
[{"sensor_id": "s4", "value": ""}]
EOF
)
if [ "$result" = "1" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 1"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --not-null does not filter missing columns (only filters if column exists with null value)
echo ""
echo "Test: --not-null does not filter rows with missing column"
result=$(cat <<'EOF' | ./sensor-data count --not-null value
[{"sensor_id": "s1", "value": "10"}]
[{"sensor_id": "s2"}]
[{"sensor_id": "s3", "value": "null"}]
EOF
)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2 (missing column is not filtered, only 'null' value is)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --tail with CSV file
echo ""
echo "Test: --tail with CSV input file"
mkdir -p testdir
cat > testdir/tail_test.csv << 'EOF'
sensor,timestamp,value
ds18b20,1000,22.5
ds18b20,2000,23.0
ds18b20,3000,23.5
ds18b20,4000,24.0
ds18b20,5000,24.5
EOF
# With --tail 2, should count only last 2 data rows (not header)
result=$(./sensor-data count --tail 2 testdir/tail_test.csv)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi
rm -rf testdir

# Test: --tail with CSV file excludes header from count
echo ""
echo "Test: --tail with CSV file excludes header"
mkdir -p testdir
cat > testdir/tail_header.csv << 'EOF'
sensor,timestamp,value
ds18b20,1000,22.5
EOF
# With --tail 5, should still count only 1 row (header should not be counted)
result=$(./sensor-data count --tail 5 testdir/tail_header.csv)
if [ "$result" = "1" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 1 (header should be excluded)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi
rm -rf testdir

# Test: --tail with CSV and date filter
echo ""
echo "Test: --tail with CSV and date filter"
mkdir -p testdir
cat > testdir/tail_date.csv << 'EOF'
sensor,timestamp,value
ds18b20,1000,22.5
ds18b20,2000,23.0
ds18b20,3000,23.5
EOF
# With --tail 2 and --min-date 2500, should count only readings after timestamp 2500
result=$(./sensor-data count --tail 2 --min-date 2500 testdir/tail_date.csv)
if [ "$result" = "1" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 1 (only timestamp 3000 passes filter)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi
rm -rf testdir

# Test: --follow with stdin (JSON)
echo ""
echo "Test: --follow with stdin JSON input"
if [ -n "$TIMEOUT_CMD" ]; then
    # Use timeout to kill the infinite loop after getting output
    # Feed 3 JSON lines and check that count updates
    result=$(echo -e '{"sensor":"a","value":1}\n{"sensor":"b","value":2}\n{"sensor":"c","value":3}' | $TIMEOUT_CMD 1 ./sensor-data count --follow 2>/dev/null || true)
    # Should output: 0, 1, 2, 3 (initial 0 then increment for each reading)
    if echo "$result" | grep -q "^0$" && echo "$result" | grep -q "^3$"; then
        echo "  ✓ PASS"
        PASSED=$((PASSED + 1))
    else
        echo "  ✗ FAIL"
        echo "  Expected output containing 0 and 3"
        echo "  Got: $result"
        FAILED=$((FAILED + 1))
    fi
else
    echo "  ⊘ SKIP (timeout command not available)"
fi

# Test: --follow with stdin (CSV)
echo ""
echo "Test: --follow with stdin CSV input"
if [ -n "$TIMEOUT_CMD" ]; then
    csv_input="sensor,value
a,1
b,2"
    result=$(echo "$csv_input" | $TIMEOUT_CMD 1 ./sensor-data count --follow -if csv 2>/dev/null || true)
    # Should output: 0, 1, 2 (initial 0, then increment for each data row, header skipped)
    if echo "$result" | grep -q "^0$" && echo "$result" | grep -q "^2$"; then
        echo "  ✓ PASS"
        PASSED=$((PASSED + 1))
    else
        echo "  ✗ FAIL"
        echo "  Expected output containing 0 and 2"
        echo "  Got: $result"
        FAILED=$((FAILED + 1))
    fi
else
    echo "  ⊘ SKIP (timeout command not available)"
fi

# Test: --follow with file (JSON)
echo ""
echo "Test: --follow with JSON file"
if [ -n "$TIMEOUT_CMD" ]; then
    mkdir -p testdir
    echo '{"sensor":"a","value":1}' > testdir/follow.out
    echo '{"sensor":"b","value":2}' >> testdir/follow.out
    # Follow mode reads existing content then waits - timeout kills it
    result=$($TIMEOUT_CMD 1 ./sensor-data count --follow testdir/follow.out 2>/dev/null || true)
    # Should output: 2 (count of existing readings)
    if echo "$result" | grep -q "^2$"; then
        echo "  ✓ PASS"
        PASSED=$((PASSED + 1))
    else
        echo "  ✗ FAIL"
        echo "  Expected output containing 2"
        echo "  Got: $result"
        FAILED=$((FAILED + 1))
    fi
    rm -rf testdir
else
    echo "  ⊘ SKIP (timeout command not available)"
fi

# Test: --follow with file (CSV)
echo ""
echo "Test: --follow with CSV file"
if [ -n "$TIMEOUT_CMD" ]; then
    mkdir -p testdir
    cat > testdir/follow.csv << 'EOF'
sensor,timestamp,value
ds18b20,1000,22.5
ds18b20,2000,23.0
ds18b20,3000,23.5
EOF
    result=$($TIMEOUT_CMD 1 ./sensor-data count --follow testdir/follow.csv 2>/dev/null || true)
    # Should output: 3 (count of existing data rows, header excluded)
    if echo "$result" | grep -q "^3$"; then
        echo "  ✓ PASS"
        PASSED=$((PASSED + 1))
    else
        echo "  ✗ FAIL"
        echo "  Expected output containing 3"
        echo "  Got: $result"
        FAILED=$((FAILED + 1))
    fi
    rm -rf testdir
else
    echo "  ⊘ SKIP (timeout command not available)"
fi

# Test: --follow with stdin JSON and filter
echo ""
echo "Test: --follow with stdin JSON and --only-value filter"
if [ -n "$TIMEOUT_CMD" ]; then
    result=$(echo -e '{"sensor":"a","value":1}\n{"sensor":"b","value":2}\n{"sensor":"a","value":3}' | $TIMEOUT_CMD 1 ./sensor-data count --follow --only-value sensor:a 2>/dev/null || true)
    # Should count only sensor:a readings (2 of them)
    if echo "$result" | grep -q "^2$"; then
        echo "  ✓ PASS"
        PASSED=$((PASSED + 1))
    else
        echo "  ✗ FAIL"
        echo "  Expected output containing 2 (only sensor:a readings)"
        echo "  Got: $result"
        FAILED=$((FAILED + 1))
    fi
else
    echo "  ⊘ SKIP (timeout command not available)"
fi

# Test: --by-month basic functionality
echo ""
echo "Test: --by-month counts by month"
input='{"timestamp": 1704067200, "value": 1}
{"timestamp": 1704153600, "value": 2}
{"timestamp": 1706745600, "value": 3}'
# 1704067200 = 2024-01-01, 1704153600 = 2024-01-02, 1706745600 = 2024-02-01
result=$(echo "$input" | ./sensor-data count --by-month)
if echo "$result" | grep -q "2024-01" && echo "$result" | grep -q "2024-02"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected output containing 2024-01 and 2024-02"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --by-month ascending order
echo ""
echo "Test: --by-month output is in ascending order"
input='{"timestamp": 1706745600, "value": 1}
{"timestamp": 1704067200, "value": 2}
{"timestamp": 1709424000, "value": 3}'
# 1704067200 = 2024-01, 1706745600 = 2024-02, 1709424000 = 2024-03
result=$(echo "$input" | ./sensor-data count --by-month)
first_month=$(echo "$result" | head -1 | awk '{print $1}')
if [ "$first_month" = "2024-01" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected first month to be 2024-01"
    echo "  Got first line: $first_month"
    FAILED=$((FAILED + 1))
fi

# Test: --by-month with CSV output
echo ""
echo "Test: --by-month with CSV output format"
input='{"timestamp": 1704067200, "value": 1}
{"timestamp": 1704153600, "value": 2}'
result=$(echo "$input" | ./sensor-data count --by-month -of csv)
header=$(echo "$result" | head -1)
if [ "$header" = "month,count" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected header: month,count"
    echo "  Got: $header"
    FAILED=$((FAILED + 1))
fi

# Test: --by-month with JSON output
echo ""
echo "Test: --by-month with JSON output format"
input='{"timestamp": 1704067200, "value": 1}'
result=$(echo "$input" | ./sensor-data count --by-month -of json)
if echo "$result" | grep -q '"month"' && echo "$result" | grep -q '"count"'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected JSON with month and count keys"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --by-month counts correctly
echo ""
echo "Test: --by-month counts correct number per month"
input='{"timestamp": 1704067200, "value": 1}
{"timestamp": 1704153600, "value": 2}
{"timestamp": 1706745600, "value": 3}'
# 2 readings in 2024-01, 1 reading in 2024-02
result=$(echo "$input" | ./sensor-data count --by-month -of csv | grep "2024-01" | cut -d',' -f2)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected 2024-01 count to be 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --by-day basic functionality
echo ""
echo "Test: --by-day counts by day (YYYY-MM-DD)"
input='{"timestamp": 1704067200, "value": 1}
{"timestamp": 1704067260, "value": 2}
{"timestamp": 1704153600, "value": 3}'
# 1704067200 = 2024-01-01 00:00:00 UTC, 1704067260 = 2024-01-01 00:01:00 UTC, 1704153600 = 2024-01-02 00:00:00 UTC
result=$(echo "$input" | ./sensor-data count --by-day)
if echo "$result" | grep -q "2024-01-01" && echo "$result" | grep -q "2024-01-02"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected output containing 2024-01-01 and 2024-01-02"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --by-day counts correctly
echo ""
echo "Test: --by-day counts correct number per day"
input='{"timestamp": 1704067200, "value": 1}
{"timestamp": 1704067260, "value": 2}
{"timestamp": 1704153600, "value": 3}'
# 2 readings on 2024-01-01, 1 reading on 2024-01-02
result=$(echo "$input" | ./sensor-data count --by-day -of csv | grep "2024-01-01" | cut -d',' -f2)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected 2024-01-01 count to be 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --by-day with CSV output
echo ""
echo "Test: --by-day with CSV output format"
input='{"timestamp": 1704067200, "value": 1}'
result=$(echo "$input" | ./sensor-data count --by-day -of csv)
header=$(echo "$result" | head -1)
if [ "$header" = "day,count" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected header: day,count"
    echo "  Got: $header"
    FAILED=$((FAILED + 1))
fi

# Test: --by-year basic functionality
echo ""
echo "Test: --by-year counts by year (YYYY)"
input='{"timestamp": 1704067200, "value": 1}
{"timestamp": 1704153600, "value": 2}
{"timestamp": 1672531200, "value": 3}'
# 1704067200 = 2024-01-01, 1704153600 = 2024-01-02, 1672531200 = 2023-01-01
result=$(echo "$input" | ./sensor-data count --by-year)
if echo "$result" | grep -q "2024" && echo "$result" | grep -q "2023"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected output containing 2023 and 2024"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --by-year counts correctly
echo ""
echo "Test: --by-year counts correct number per year"
input='{"timestamp": 1704067200, "value": 1}
{"timestamp": 1704153600, "value": 2}
{"timestamp": 1672531200, "value": 3}'
# 2 readings in 2024, 1 reading in 2023
result=$(echo "$input" | ./sensor-data count --by-year -of csv | grep "2024" | cut -d',' -f2)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected 2024 count to be 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --by-year with CSV output
echo ""
echo "Test: --by-year with CSV output format"
input='{"timestamp": 1704067200, "value": 1}'
result=$(echo "$input" | ./sensor-data count --by-year -of csv)
header=$(echo "$result" | head -1)
if [ "$header" = "year,count" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected header: year,count"
    echo "  Got: $header"
    FAILED=$((FAILED + 1))
fi

# Test: --by-week basic functionality
echo ""
echo "Test: --by-week counts by week (YYYY-WNN)"
input='{"timestamp": 1704067200, "value": 1}
{"timestamp": 1704672000, "value": 2}
{"timestamp": 1705276800, "value": 3}'
# 1704067200 = 2024-01-01 (week 1), 1704672000 = 2024-01-08 (week 2), 1705276800 = 2024-01-15 (week 3)
result=$(echo "$input" | ./sensor-data count --by-week)
if echo "$result" | grep -qE "2024-W0[123]"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected output containing week format like 2024-W01"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --by-week with CSV output
echo ""
echo "Test: --by-week with CSV output format"
input='{"timestamp": 1704067200, "value": 1}'
result=$(echo "$input" | ./sensor-data count --by-week -of csv)
header=$(echo "$result" | head -1)
if [ "$header" = "week,count" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected header: week,count"
    echo "  Got: $header"
    FAILED=$((FAILED + 1))
fi

# Test: --by-week counts correctly
echo ""
echo "Test: --by-week counts correct number per week"
input='{"timestamp": 1704067200, "value": 1}
{"timestamp": 1704153600, "value": 2}
{"timestamp": 1704672000, "value": 3}'
# 2 readings in week 1, 1 reading in week 2
result=$(echo "$input" | ./sensor-data count --by-week -of csv | grep "2024-W01" | cut -d',' -f2)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected 2024-W01 count to be 2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: -o/--output writes to file
echo ""
echo "Test: -o writes output to file"
input='{"timestamp": 1704067200, "value": 1}
{"timestamp": 1704153600, "value": 2}'
tmpfile=$(mktemp)
echo "$input" | ./sensor-data count --by-month -of csv -o "$tmpfile"
if [ -f "$tmpfile" ] && grep -q "month,count" "$tmpfile" && grep -q "2024-01" "$tmpfile"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected file to contain month,count header and 2024-01 data"
    echo "  Got: $(cat $tmpfile)"
    FAILED=$((FAILED + 1))
fi
rm -f "$tmpfile"

# Test: --output writes correct count to file
echo ""
echo "Test: --output writes correct count to file"
input='{"timestamp": 1704067200, "value": 1}
{"timestamp": 1704153600, "value": 2}
{"timestamp": 1704240000, "value": 3}'
tmpfile=$(mktemp)
echo "$input" | ./sensor-data count -o "$tmpfile"
result=$(cat "$tmpfile")
if [ "$result" = "3" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected file to contain: 3"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi
rm -f "$tmpfile"

# Test: mutual exclusivity of time grouping options
echo ""
echo "Test: Multiple time grouping options gives error"
input='{"timestamp": 1704067200, "value": 1}'
result=$(echo "$input" | ./sensor-data count --by-month --by-day 2>&1) || true
if echo "$result" | grep -qi "error\|cannot\|mutually"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected error when using multiple time grouping options"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --unique removes duplicate rows from count
echo ""
echo "Test: --unique removes duplicate rows from count"
result=$(cat <<'EOF' | ./sensor-data count --unique
[{"sensor": "ds18b20", "value": 22.5}]
[{"sensor": "ds18b20", "value": 22.5}]
[{"sensor": "ds18b20", "value": 23.0}]
[{"sensor": "ds18b20", "value": 22.5}]
EOF
)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2 (unique rows only)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --clean includes --unique
echo ""
echo "Test: --clean includes --unique (removes duplicates)"
result=$(cat <<'EOF' | ./sensor-data count --clean
[{"sensor": "ds18b20", "value": 22.5, "sensor_id": "s1"}]
[{"sensor": "ds18b20", "value": 22.5, "sensor_id": "s1"}]
[{"sensor": "ds18b20", "value": 23.0, "sensor_id": "s1"}]
EOF
)
if [ "$result" = "2" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: 2 (--clean enables --unique)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: --unique with --by-column
echo ""
echo "Test: --unique with --by-column counts unique rows per value"
result=$(cat <<'EOF' | ./sensor-data count --unique --by-column sensor
[{"sensor": "ds18b20", "value": 22.5}]
[{"sensor": "ds18b20", "value": 22.5}]
[{"sensor": "dht22", "value": 45}]
[{"sensor": "ds18b20", "value": 23.0}]
[{"sensor": "dht22", "value": 45}]
EOF
)
ds18b20_count=$(echo "$result" | grep "ds18b20" | awk '{print $2}')
dht22_count=$(echo "$result" | grep "dht22" | awk '{print $2}')
if [ "$ds18b20_count" = "2" ] && [ "$dht22_count" = "1" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Expected: ds18b20=2, dht22=1"
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
