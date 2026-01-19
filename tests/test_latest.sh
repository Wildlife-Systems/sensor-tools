#!/bin/bash
# Integration tests for latest functionality

set -e  # Exit on error

echo "================================"
echo "Testing latest functionality"
echo "================================"

# Build sensor-data if not already built
if [ ! -f sensor-data ]; then
    echo "Building sensor-data..."
    make sensor-data 2>/dev/null || g++ -std=c++11 -pthread -Iinclude -o sensor-data src/*.cpp
fi

PASSED=0
FAILED=0

# Test: latest --help shows usage
echo "Test: latest --help shows usage"
result=$(./sensor-data latest --help 2>&1) || true
if echo "$result" | grep -q -i "latest\|usage"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected latest help text"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Create temp test file
TESTFILE=$(mktemp)
trap "rm -f $TESTFILE" EXIT

# Test 1: Basic latest output (human-readable default)
echo ""
echo "Test 1: Basic latest output (human-readable)"
cat > "$TESTFILE" <<'EOF'
{"sensor_id":"sensor1","timestamp":"1737315600","temperature":"22.5"}
{"sensor_id":"sensor1","timestamp":"1737315700","temperature":"23.0"}
{"sensor_id":"sensor2","timestamp":"1737315650","temperature":"18.0"}
EOF
result=$(./sensor-data latest "$TESTFILE")
if echo "$result" | grep -q "sensor1" && echo "$result" | grep -q "sensor2" && echo "$result" | grep -q "Latest readings"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 2: Latest picks the newest timestamp per sensor
echo ""
echo "Test 2: Latest picks newest timestamp per sensor"
result=$(./sensor-data latest -of csv "$TESTFILE")
# sensor1 should have timestamp 1737315700 (the later one), not 1737315600
if echo "$result" | grep -q "sensor1,1737315700"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected sensor1 to have timestamp 1737315700"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 3: CSV output format
echo ""
echo "Test 3: CSV output format (-of csv)"
result=$(./sensor-data latest -of csv "$TESTFILE")
if echo "$result" | head -1 | grep -q "sensor_id,timestamp,iso_date" && echo "$result" | grep -q "sensor1,"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 4: JSON output format
echo ""
echo "Test 4: JSON output format (-of json)"
result=$(./sensor-data latest -of json "$TESTFILE")
if echo "$result" | grep -q '"sensor_id":"sensor1"' && echo "$result" | grep -q '"timestamp":1737315700'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 5: -n positive (first n sensors)
echo ""
echo "Test 5: -n positive (first n sensors)"
result=$(./sensor-data latest -of csv -n 1 "$TESTFILE")
# Should only have 1 data row (plus header)
linecount=$(echo "$result" | wc -l)
if [ "$linecount" -eq 2 ] && echo "$result" | grep -q "sensor1,"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 lines (header + 1 sensor)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 6: -n negative (last n sensors)
echo ""
echo "Test 6: -n negative (last n sensors)"
result=$(./sensor-data latest -of csv -n -1 "$TESTFILE")
# Should only have 1 data row (plus header), and it should be sensor2 (last alphabetically)
linecount=$(echo "$result" | wc -l)
if [ "$linecount" -eq 2 ] && echo "$result" | grep -q "sensor2,"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 lines (header + sensor2)"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 7: No input files error
echo ""
echo "Test 7: No input files shows error"
result=$(./sensor-data latest 2>&1) || true
if echo "$result" | grep -qi "error.*no input"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected error about no input files"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 8: Empty file
echo ""
echo "Test 8: Empty file produces no results"
echo "" > "$TESTFILE"
result=$(./sensor-data latest "$TESTFILE")
if echo "$result" | grep -q "Total: 0 sensor"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 9: Missing sensor_id field
echo ""
echo "Test 9: Missing sensor_id field is skipped"
cat > "$TESTFILE" <<'EOF'
{"timestamp":"1737315600","temperature":"22.5"}
{"sensor_id":"sensor1","timestamp":"1737315700","temperature":"23.0"}
EOF
result=$(./sensor-data latest -of csv "$TESTFILE")
# Should only have sensor1
linecount=$(echo "$result" | wc -l)
if [ "$linecount" -eq 2 ] && echo "$result" | grep -q "sensor1,"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected only sensor1"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 10: Missing timestamp field is skipped
echo ""
echo "Test 10: Missing timestamp field is skipped"
cat > "$TESTFILE" <<'EOF'
{"sensor_id":"sensor1","temperature":"22.5"}
{"sensor_id":"sensor2","timestamp":"1737315700","temperature":"23.0"}
EOF
result=$(./sensor-data latest -of csv "$TESTFILE")
# Should only have sensor2
linecount=$(echo "$result" | wc -l)
if [ "$linecount" -eq 2 ] && echo "$result" | grep -q "sensor2,"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected only sensor2"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 11: CSV input format
echo ""
echo "Test 11: CSV input format (-if csv)"
cat > "$TESTFILE" <<'EOF'
sensor_id,timestamp,temperature
sensor1,1737315600,22.5
sensor1,1737315700,23.0
sensor2,1737315650,18.0
EOF
result=$(./sensor-data latest -if csv -of csv "$TESTFILE")
if echo "$result" | grep -q "sensor1,1737315700"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 12: Date filtering with --min-date
echo ""
echo "Test 12: Date filtering with --min-date"
cat > "$TESTFILE" <<'EOF'
{"sensor_id":"sensor1","timestamp":"1737315600","temperature":"22.5"}
{"sensor_id":"sensor1","timestamp":"1737315700","temperature":"23.0"}
{"sensor_id":"sensor2","timestamp":"1737315650","temperature":"18.0"}
EOF
# Timestamp 1737315700 = 2025-01-19 19:41:40 (approx)
# Filter to only include readings >= 1737315680
result=$(./sensor-data latest -of csv --min-date 1737315680 "$TESTFILE")
# Only sensor1's second reading (1737315700) should pass
if echo "$result" | grep -q "sensor1,1737315700" && ! echo "$result" | grep -q "sensor2"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected only sensor1 with timestamp >= 1737315680"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 13: Date filtering with --max-date
echo ""
echo "Test 13: Date filtering with --max-date"
cat > "$TESTFILE" <<'EOF'
{"sensor_id":"sensor1","timestamp":"1737315600","temperature":"22.5"}
{"sensor_id":"sensor1","timestamp":"1737315700","temperature":"23.0"}
{"sensor_id":"sensor2","timestamp":"1737315650","temperature":"18.0"}
EOF
# Filter to only include readings <= 1737315650
result=$(./sensor-data latest -of csv --max-date 1737315650 "$TESTFILE")
if echo "$result" | grep -q "sensor1,1737315600" && echo "$result" | grep -q "sensor2,1737315650"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected sensor1=1737315600 and sensor2=1737315650"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 14: Multiple sensors, many readings
echo ""
echo "Test 14: Multiple sensors with many readings"
cat > "$TESTFILE" <<'EOF'
{"sensor_id":"a","timestamp":"100","value":"1"}
{"sensor_id":"b","timestamp":"200","value":"2"}
{"sensor_id":"c","timestamp":"300","value":"3"}
{"sensor_id":"a","timestamp":"400","value":"4"}
{"sensor_id":"b","timestamp":"150","value":"5"}
{"sensor_id":"c","timestamp":"500","value":"6"}
{"sensor_id":"a","timestamp":"350","value":"7"}
EOF
result=$(./sensor-data latest -of csv "$TESTFILE")
# a should have 400, b should have 200, c should have 500
if echo "$result" | grep -q "a,400" && echo "$result" | grep -q "b,200" && echo "$result" | grep -q "c,500"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 15: Sensors are sorted alphabetically
echo ""
echo "Test 15: Sensors are sorted alphabetically"
cat > "$TESTFILE" <<'EOF'
{"sensor_id":"zebra","timestamp":"100","value":"1"}
{"sensor_id":"alpha","timestamp":"200","value":"2"}
{"sensor_id":"middle","timestamp":"300","value":"3"}
EOF
result=$(./sensor-data latest -of csv "$TESTFILE" | tail -n +2)
# Should be: alpha, middle, zebra
first=$(echo "$result" | head -1 | cut -d, -f1)
last=$(echo "$result" | tail -1 | cut -d, -f1)
if [ "$first" = "alpha" ] && [ "$last" = "zebra" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected alpha first, zebra last"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 16: Single sensor
echo ""
echo "Test 16: Single sensor"
cat > "$TESTFILE" <<'EOF'
{"sensor_id":"only_one","timestamp":"12345","value":"1"}
EOF
result=$(./sensor-data latest -of csv "$TESTFILE")
if echo "$result" | grep -q "only_one,12345" && [ "$(echo "$result" | wc -l)" -eq 2 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 17: -n larger than number of sensors
echo ""
echo "Test 17: -n larger than number of sensors"
cat > "$TESTFILE" <<'EOF'
{"sensor_id":"a","timestamp":"100","value":"1"}
{"sensor_id":"b","timestamp":"200","value":"2"}
EOF
result=$(./sensor-data latest -of csv -n 10 "$TESTFILE")
# Should show all 2 sensors
datalines=$(echo "$result" | tail -n +2 | wc -l)
if [ "$datalines" -eq 2 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 2 data lines"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 18: -n 0 means all
echo ""
echo "Test 18: -n 0 means all"
cat > "$TESTFILE" <<'EOF'
{"sensor_id":"a","timestamp":"100","value":"1"}
{"sensor_id":"b","timestamp":"200","value":"2"}
{"sensor_id":"c","timestamp":"300","value":"3"}
EOF
result=$(./sensor-data latest -of csv -n 0 "$TESTFILE")
datalines=$(echo "$result" | tail -n +2 | wc -l)
if [ "$datalines" -eq 3 ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 3 data lines"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 19: Verbose output
echo ""
echo "Test 19: Verbose output (-v)"
cat > "$TESTFILE" <<'EOF'
{"sensor_id":"a","timestamp":"100","value":"1"}
EOF
result=$(./sensor-data latest -v "$TESTFILE" 2>&1)
if echo "$result" | grep -qi "processing\|verbose\|file"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected verbose output"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 20: --tail option
echo ""
echo "Test 20: --tail option"
cat > "$TESTFILE" <<'EOF'
{"sensor_id":"old","timestamp":"100","value":"1"}
{"sensor_id":"old","timestamp":"200","value":"2"}
{"sensor_id":"old","timestamp":"300","value":"3"}
{"sensor_id":"new","timestamp":"400","value":"4"}
EOF
# With --tail 1, only last line should be read
result=$(./sensor-data latest -of csv --tail 1 "$TESTFILE")
if echo "$result" | grep -q "new,400" && ! echo "$result" | grep -q "old,"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected only 'new' sensor from last line"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 21: ISO date format in timestamp field
echo ""
echo "Test 21: ISO date format in timestamp field"
cat > "$TESTFILE" <<'EOF'
{"sensor_id":"sensor1","timestamp":"2025-01-19T10:00:00","value":"1"}
{"sensor_id":"sensor1","timestamp":"2025-01-19T12:00:00","value":"2"}
EOF
result=$(./sensor-data latest -of csv "$TESTFILE")
# The second timestamp should be picked (later time)
if echo "$result" | grep -q "sensor1," && echo "$result" | grep -q "2025-01-19 12:00:00"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 22: Human output shows total count
echo ""
echo "Test 22: Human output shows total count"
cat > "$TESTFILE" <<'EOF'
{"sensor_id":"a","timestamp":"100","value":"1"}
{"sensor_id":"b","timestamp":"200","value":"2"}
{"sensor_id":"c","timestamp":"300","value":"3"}
EOF
result=$(./sensor-data latest "$TESTFILE")
if echo "$result" | grep -q "Total: 3 sensor"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected 'Total: 3 sensor'"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 23: JSON array output is valid
echo ""
echo "Test 23: JSON array output is valid"
cat > "$TESTFILE" <<'EOF'
{"sensor_id":"a","timestamp":"100","value":"1"}
{"sensor_id":"b","timestamp":"200","value":"2"}
EOF
result=$(./sensor-data latest -of json "$TESTFILE")
# Should start with [ and end with ]
if [[ "$result" == "["* ]] && [[ "$result" == *"]" ]]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected JSON array"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 24: JSON output with single sensor
echo ""
echo "Test 24: JSON output with single sensor"
cat > "$TESTFILE" <<'EOF'
{"sensor_id":"only","timestamp":"12345","value":"1"}
EOF
result=$(./sensor-data latest -of json "$TESTFILE")
if echo "$result" | grep -q '^\[{"sensor_id":"only"'; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test 25: Empty JSON output for no results
echo ""
echo "Test 25: JSON output with no valid sensors"
cat > "$TESTFILE" <<'EOF'
{"no_sensor_id":"missing","timestamp":"100","value":"1"}
EOF
result=$(./sensor-data latest -of json "$TESTFILE")
if [ "$result" = "[]" ]; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Expected empty JSON array"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

echo ""
echo "================================"
echo "Latest tests: $PASSED passed, $FAILED failed"
echo "================================"

if [ $FAILED -gt 0 ]; then
    exit 1
fi
