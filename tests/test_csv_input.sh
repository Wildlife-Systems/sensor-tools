#!/bin/bash
# Integration tests for CSV file input

set -e  # Exit on error

echo "================================"
echo "Testing CSV file input"
echo "================================"

# Build sensor-data if not already built
if [ ! -f sensor-data ]; then
    echo "Building sensor-data..."
    make sensor-data
fi

PASSED=0
FAILED=0

# Create temporary directory for test files
TESTDIR=$(mktemp -d)
trap "rm -rf $TESTDIR" EXIT

# Create test CSV files
cat > "$TESTDIR/sensor1.csv" << 'EOF'
sensor,value,sensor_id,timestamp
ds18b20,22.5,001,2024-01-15 10:00:00
ds18b20,23.0,001,2024-01-15 11:00:00
ds18b20,21.5,001,2024-01-15 12:00:00
EOF

cat > "$TESTDIR/sensor2.csv" << 'EOF'
sensor,value,sensor_id,timestamp
ds18b20,24.0,002,2024-01-15 10:00:00
ds18b20,25.0,002,2024-01-15 11:00:00
EOF

cat > "$TESTDIR/errors.csv" << 'EOF'
sensor,value,sensor_id,timestamp
ds18b20,85,001,2024-01-15 10:00:00
ds18b20,22.5,001,2024-01-15 11:00:00
ds18b20,-127,001,2024-01-15 12:00:00
ds18b20,85,002,2024-01-15 10:00:00
EOF

# Create nested directory structure
mkdir -p "$TESTDIR/subdir1"
mkdir -p "$TESTDIR/subdir2"

cat > "$TESTDIR/subdir1/data1.csv" << 'EOF'
sensor,value,sensor_id,timestamp
ds18b20,20.0,003,2024-01-15 10:00:00
ds18b20,20.5,003,2024-01-15 11:00:00
EOF

cat > "$TESTDIR/subdir2/data2.csv" << 'EOF'
sensor,value,sensor_id,timestamp
ds18b20,26.0,004,2024-01-15 10:00:00
ds18b20,26.5,004,2024-01-15 11:00:00
EOF

echo "Created test files in $TESTDIR"

# ============================================
# transform COMMAND TESTS
# ============================================

echo ""
echo "--- transform command tests ---"

# Test: Single CSV file input
echo ""
echo "Test: Single CSV file input"
result=$(./sensor-data transform "$TESTDIR/sensor1.csv")
if echo "$result" | grep -q "22.5" && echo "$result" | grep -q "23.0" && echo "$result" | grep -q "21.5"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected all three values in output"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Multiple CSV files input
echo ""
echo "Test: Multiple CSV files input"
result=$(./sensor-data transform "$TESTDIR/sensor1.csv" "$TESTDIR/sensor2.csv")
if echo "$result" | grep -q "22.5" && echo "$result" | grep -q "25.0"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected values from both files"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: CSV directory input
echo ""
echo "Test: CSV directory input (non-recursive)"
result=$(./sensor-data transform "$TESTDIR")
count=$(echo "$result" | grep -c "ds18b20" || true)
# Should find files in root only: sensor1.csv (3), sensor2.csv (2), errors.csv (4) = 9 records
if [ "$count" -eq 9 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 9 records, got $count"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: CSV directory input recursive
echo ""
echo "Test: CSV directory input (recursive)"
result=$(./sensor-data transform -r "$TESTDIR")
count=$(echo "$result" | grep -c "ds18b20" || true)
# Should find all files: root (9) + subdir1 (2) + subdir2 (2) = 13 records
if [ "$count" -eq 13 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 13 records, got $count"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: CSV file with format conversion to JSON
echo ""
echo "Test: CSV input to JSON output (default)"
result=$(./sensor-data transform "$TESTDIR/sensor1.csv")
if echo "$result" | grep -q '"sensor"' && echo "$result" | grep -q '"value"'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected JSON output"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: CSV file with format conversion to CSV
echo ""
echo "Test: CSV input to CSV output"
result=$(./sensor-data transform -F csv "$TESTDIR/sensor1.csv")
if echo "$result" | head -1 | grep -q "sensor" && echo "$result" | head -1 | grep -q "value"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected CSV header with sensor and value columns"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: CSV file with remove-errors flag
echo ""
echo "Test: CSV input with --remove-errors"
result=$(./sensor-data transform --remove-errors "$TESTDIR/errors.csv")
if echo "$result" | grep -q "22.5" && ! echo "$result" | grep -q '"value":"85"' && ! echo "$result" | grep -q '"value":"-127"'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected error values to be removed"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: CSV file with sensor filter
echo ""
echo "Test: CSV input with sensor filter"
result=$(./sensor-data transform -s ds18b20 "$TESTDIR/sensor1.csv")
count=$(echo "$result" | grep -c "ds18b20" || true)
if [ "$count" -eq 3 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 3 records"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: CSV file with only-value filter
echo ""
echo "Test: CSV input with --only-value filter"
result=$(./sensor-data transform --only-value value:22.5 "$TESTDIR/sensor1.csv")
count=$(echo "$result" | grep -c "22.5" || true)
if [ "$count" -eq 1 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 1 record with value 22.5"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# ============================================
# STATS COMMAND TESTS
# ============================================

echo ""
echo "--- Stats command tests ---"

# Test: Stats on single CSV file
echo ""
echo "Test: Stats on single CSV file"
result=$(./sensor-data stats "$TESTDIR/sensor1.csv")
if echo "$result" | grep -q "value:" && echo "$result" | grep -q "Count:.*3" && echo "$result" | grep -q "Min:.*21.5" && echo "$result" | grep -q "Max:.*23"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected stats for 3 values"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Stats on multiple CSV files
echo ""
echo "Test: Stats on multiple CSV files"
result=$(./sensor-data stats "$TESTDIR/sensor1.csv" "$TESTDIR/sensor2.csv")
if echo "$result" | grep -q "Count:.*5"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected stats for 5 values"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Stats on CSV directory
echo ""
echo "Test: Stats on CSV directory"
result=$(./sensor-data stats "$TESTDIR")
if echo "$result" | grep -q "Count:.*9"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected stats for 9 values (non-recursive)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Stats on CSV directory recursive
echo ""
echo "Test: Stats on CSV directory (recursive)"
result=$(./sensor-data stats -r "$TESTDIR")
if echo "$result" | grep -q "Count:.*13"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected stats for 13 values"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Stats with column filter on CSV file
echo ""
echo "Test: Stats with column filter on CSV file"
result=$(./sensor-data stats -c value "$TESTDIR/sensor1.csv")
if echo "$result" | grep -q "value:" && echo "$result" | grep -q "Count:.*3"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected stats for value column"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# ============================================
# LIST-ERRORS COMMAND TESTS
# ============================================

echo ""
echo "--- List-errors command tests ---"

# Test: List-errors on single CSV file
echo ""
echo "Test: List-errors on single CSV file"
result=$(./sensor-data list-errors "$TESTDIR/errors.csv")
if echo "$result" | grep -q "value=85" && echo "$result" | grep -q "value=-127"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected error values 85 and -127"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: List-errors shows correct error count
echo ""
echo "Test: List-errors shows 3 errors from CSV"
result=$(./sensor-data list-errors "$TESTDIR/errors.csv")
count=$(echo "$result" | grep -c ":" || true)
if [ "$count" -eq 3 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 3 error lines (2x85, 1x-127)"
    echo "  Got $count lines: $result"
    FAILED=$((FAILED + 1))
fi

# Test: List-errors on CSV file with no errors
echo ""
echo "Test: List-errors on CSV file with no errors"
result=$(./sensor-data list-errors "$TESTDIR/sensor1.csv")
if [ -z "$result" ] || echo "$result" | grep -q "No errors found"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected no errors or 'No errors found'"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: List-errors on CSV directory
echo ""
echo "Test: List-errors on CSV directory"
result=$(./sensor-data list-errors "$TESTDIR")
if echo "$result" | grep -q "value=85" && echo "$result" | grep -q "value=-127"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected errors from errors.csv"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# ============================================
# SUMMARISE-ERRORS COMMAND TESTS
# ============================================

echo ""
echo "--- Summarise-errors command tests ---"

# Test: Summarise-errors on single CSV file
echo ""
echo "Test: Summarise-errors on single CSV file"
result=$(./sensor-data summarise-errors "$TESTDIR/errors.csv")
if echo "$result" | grep -q "communication error.*2 occurrence" && echo "$result" | grep -q "disconnected.*1 occurrence" && echo "$result" | grep -q "Total errors: 3"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected error summary"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Summarise-errors on CSV file with no errors
echo ""
echo "Test: Summarise-errors on CSV file with no errors"
result=$(./sensor-data summarise-errors "$TESTDIR/sensor1.csv")
if echo "$result" | grep -q "No errors found" || echo "$result" | grep -q "Total errors: 0"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected no errors message"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Summarise-errors on CSV directory
echo ""
echo "Test: Summarise-errors on CSV directory"
result=$(./sensor-data summarise-errors "$TESTDIR")
if echo "$result" | grep -q "Total errors: 3"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected total errors from errors.csv"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Summarise-errors on CSV directory recursive
echo ""
echo "Test: Summarise-errors on CSV directory (recursive)"
result=$(./sensor-data summarise-errors -r "$TESTDIR")
if echo "$result" | grep -q "Total errors: 3"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected total errors: 3"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# ============================================
# EDGE CASES
# ============================================

echo ""
echo "--- Edge cases ---"

# Test: Empty CSV file (header only)
cat > "$TESTDIR/empty.csv" << 'EOF'
sensor,value,sensor_id,timestamp
EOF

echo ""
echo "Test: Empty CSV file (header only)"
result=$(./sensor-data transform "$TESTDIR/empty.csv")
# Should produce empty output or just headers
if [ -z "$result" ] || echo "$result" | grep -q "^\[\]$" || ! echo "$result" | grep -q "ds18b20"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected empty output"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: CSV file with mixed content (some errors, some valid)
echo ""
echo "Test: Stats excludes error values from CSV"
result=$(./sensor-data stats "$TESTDIR/errors.csv")
# Should include all numeric values including 85 and -127 in stats
if echo "$result" | grep -q "Count:.*4"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 4 values in stats"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Directory with no CSV files
mkdir -p "$TESTDIR/emptydir"
echo ""
echo "Test: Directory with no CSV files"
result=$(./sensor-data transform "$TESTDIR/emptydir" 2>&1) || true
if [ -z "$result" ] || echo "$result" | grep -qi "no.*file\|empty\|no input"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected empty or no files message"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# ============================================
# SUMMARY
# ============================================

echo ""
echo "================================"
echo "CSV Input Test Summary"
echo "================================"
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ $FAILED -gt 0 ]; then
    echo ""
    echo "SOME TESTS FAILED!"
    exit 1
else
    echo ""
    echo "All tests passed!"
    exit 0
fi
