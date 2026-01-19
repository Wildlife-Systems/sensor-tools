#!/bin/bash

# Test script for --allowed-values and --by-column interaction in count command

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Test counter
TESTS_RUN=0
TESTS_PASSED=0

# Function to print test results
print_result() {
    local test_name="$1"
    local expected="$2"
    local actual="$3"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    
    if [ "$expected" = "$actual" ]; then
        echo -e "${GREEN}✓ PASS${NC}: $test_name"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ FAIL${NC}: $test_name"
        echo "  Expected: $expected"
        echo "  Actual:   $actual"
    fi
}

# Create test data
create_test_data() {
    cat > test_data.json << 'EOF'
{"timestamp": 1234567890, "sensor_id": "sensor1", "type": "temperature", "value": 20.5, "location": "room1"}
{"timestamp": 1234567891, "sensor_id": "sensor2", "type": "temperature", "value": 21.0, "location": "room1"}
{"timestamp": 1234567892, "sensor_id": "sensor3", "type": "temperature", "value": 19.8, "location": "room2"}
{"timestamp": 1234567893, "sensor_id": "sensor1", "type": "humidity", "value": 65.0, "location": "room1"}
{"timestamp": 1234567894, "sensor_id": "sensor2", "type": "humidity", "value": 60.0, "location": "room1"}
{"timestamp": 1234567895, "sensor_id": "sensor4", "type": "temperature", "value": 22.0, "location": "room2"}
{"timestamp": 1234567896, "sensor_id": "sensor1", "type": "temperature", "value": 20.8, "location": "room1"}
{"timestamp": 1234567897, "sensor_id": "sensor5", "type": "pressure", "value": 1013.2, "location": "room3"}
EOF

    # Create allowed values files
    echo -e "sensor1\nsensor2\nsensor3" > allowed_sensors.txt
    echo -e "room1\nroom2" > allowed_locations.txt
}

# Clean up test files
cleanup() {
    rm -f test_data.json allowed_sensors.txt allowed_locations.txt
}

# Test 1: Basic count without filters
test_basic_count() {
    local result=$(./sensor-data count test_data.json 2>/dev/null)
    print_result "Basic count" "8" "$result"
}

# Test 2: Count with --by-column sensor_id (no filters)
test_by_column_sensor_id() {
    local result=$(./sensor-data count --by-column sensor_id test_data.json 2>/dev/null | wc -l)
    print_result "Count by sensor_id (number of unique sensors)" "5" "$result"
}

# Test 3: Count with --allowed-values on sensor_id, --by-column on type (different columns)
test_allowed_sensor_by_type() {
    local result=$(./sensor-data count --allowed-values sensor_id allowed_sensors.txt --by-column type test_data.json 2>/dev/null)
    local expected="humidity,2
temperature,4"
    print_result "Allowed sensors, count by type" "$expected" "$result"
}

# Test 4: Count with --allowed-values on location, --by-column on sensor_id (different columns)
test_allowed_location_by_sensor() {
    local result=$(./sensor-data count --allowed-values location allowed_locations.txt --by-column sensor_id test_data.json 2>/dev/null)
    local expected="sensor1,3
sensor2,2
sensor3,1
sensor4,1"
    print_result "Allowed locations, count by sensor_id" "$expected" "$result"
}

# Test 5: Count with --allowed-values and --by-column on same column (sensor_id)
test_allowed_and_by_same_column() {
    local result=$(./sensor-data count --allowed-values sensor_id allowed_sensors.txt --by-column sensor_id test_data.json 2>/dev/null)
    local expected="sensor1,3
sensor2,2
sensor3,1"
    print_result "Allowed and by same column (sensor_id)" "$expected" "$result"
}

# Test 6: Verify filtering works - total count with allowed values
test_total_with_allowed_sensors() {
    local result=$(./sensor-data count --allowed-values sensor_id allowed_sensors.txt test_data.json 2>/dev/null)
    print_result "Total count with allowed sensors" "6" "$result"
}

# Test 7: Verify filtering works - total count with allowed locations
test_total_with_allowed_locations() {
    local result=$(./sensor-data count --allowed-values location allowed_locations.txt test_data.json 2>/dev/null)
    print_result "Total count with allowed locations" "7" "$result"
}

# Test 8: Complex filter - allowed sensors AND by type, only temperature
test_complex_filter() {
    local result=$(./sensor-data count --allowed-values sensor_id allowed_sensors.txt --only-value type:temperature --by-column sensor_id test_data.json 2>/dev/null)
    local expected="sensor1,2
sensor2,1
sensor3,1"
    print_result "Complex filter: allowed sensors + only temperature, by sensor_id" "$expected" "$result"
}

# Main test execution
main() {
    echo "Testing --allowed-values and --by-column interaction in count command"
    echo "=================================================================="
    
    # Create test data
    create_test_data
    
    # Run tests
    test_basic_count
    test_by_column_sensor_id
    test_allowed_sensor_by_type
    test_allowed_location_by_sensor
    test_allowed_and_by_same_column
    test_total_with_allowed_sensors
    test_total_with_allowed_locations
    test_complex_filter
    
    # Print summary
    echo
    echo "=================================================================="
    echo "Tests completed: $TESTS_PASSED/$TESTS_RUN passed"
    
    if [ $TESTS_PASSED -eq $TESTS_RUN ]; then
        echo -e "${GREEN}All tests passed!${NC}"
        cleanup
        exit 0
    else
        echo -e "${RED}Some tests failed!${NC}"
        cleanup
        exit 1
    fi
}

# Run the tests
main