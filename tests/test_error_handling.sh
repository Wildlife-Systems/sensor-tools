#!/bin/bash
# Integration tests for error handling - invalid arguments and formats

set -e  # Exit on error

echo "================================"
echo "Testing error handling"
echo "================================"

# Build sensor-data if not already built
if [ ! -f sensor-data ]; then
    echo "Building sensor-data..."
    make sensor-data
fi

PASSED=0
FAILED=0

# Helper to check if command fails with expected error message
check_error() {
    local test_name="$1"
    local command="$2"
    local expected_error="$3"
    
    echo ""
    echo "Test: $test_name"
    
    result=$(eval "$command" 2>&1) || true
    if echo "$result" | grep -qi "$expected_error"; then
        echo "  ✓ PASS"
        PASSED=$((PASSED + 1))
    else
        echo "  ✗ FAIL - Expected error message containing: $expected_error"
        echo "  Got: $result"
        FAILED=$((FAILED + 1))
    fi
}

# ============================================
# INVALID DATE FORMAT ERRORS
# ============================================

echo ""
echo "--- Invalid date format errors ---"

# Test: Invalid --min-date format (convert)
check_error "convert: Invalid --min-date format" \
    "echo '{}' | ./sensor-data convert --min-date invalid-date" \
    "invalid date format"

# Test: Invalid --max-date format (convert)
check_error "convert: Invalid --max-date format" \
    "echo '{}' | ./sensor-data convert --max-date not-a-date" \
    "invalid date format"

# Test: Invalid --min-date format (stats)
check_error "stats: Invalid --min-date format" \
    "echo '{}' | ./sensor-data stats --min-date abc123" \
    "invalid date format"

# Test: Invalid --max-date format (stats)
check_error "stats: Invalid --max-date format" \
    "echo '{}' | ./sensor-data stats --max-date not-a-date" \
    "invalid date format"

# Test: Invalid date - day out of range (32)
check_error "convert: Invalid date - day 32" \
    "echo '{}' | ./sensor-data convert --min-date 2024-01-32" \
    "invalid date format"

# Test: Invalid date - month out of range (13)
check_error "convert: Invalid date - month 13" \
    "echo '{}' | ./sensor-data convert --min-date 2024-13-15" \
    "invalid date format"

# Test: Invalid date - Feb 30
check_error "convert: Invalid date - Feb 30" \
    "echo '{}' | ./sensor-data convert --min-date 2024-02-30" \
    "invalid date format"

# Test: Invalid date - Feb 29 in non-leap year
check_error "convert: Invalid date - Feb 29 in non-leap year" \
    "echo '{}' | ./sensor-data convert --min-date 2026-02-29" \
    "invalid date format"

# Test: Invalid date - invalid time (hour 25)
check_error "convert: Invalid date - hour 25" \
    "echo '{}' | ./sensor-data convert --min-date 2024-01-15T25:00:00" \
    "invalid date format"

# Test: Invalid UK format - day too large (2024/01/01 parses as day=2024)
check_error "convert: Invalid UK format - day too large" \
    "echo '{}' | ./sensor-data convert --min-date 2024/01/01" \
    "invalid date format"

# Test: Invalid --min-date format (list-errors)
check_error "list-errors: Invalid --min-date format" \
    "echo '{}' | ./sensor-data list-errors --min-date yesterday" \
    "invalid date format"

# Test: Invalid --max-date format (list-errors)
check_error "list-errors: Invalid --max-date format" \
    "echo '{}' | ./sensor-data list-errors --max-date tomorrow" \
    "invalid date format"

# Test: Invalid --min-date format (summarise-errors)
check_error "summarise-errors: Invalid --min-date format" \
    "echo '{}' | ./sensor-data summarise-errors --min-date 01-01-2024" \
    "invalid date format"

# Test: Invalid --max-date format (summarise-errors)
check_error "summarise-errors: Invalid --max-date format" \
    "echo '{}' | ./sensor-data summarise-errors --max-date Jan-15-2024" \
    "invalid date format"

# ============================================
# MISSING ARGUMENT ERRORS
# ============================================

echo ""
echo "--- Missing argument errors ---"

# Test: --min-date missing argument
check_error "convert: --min-date missing argument" \
    "echo '{}' | ./sensor-data convert --min-date" \
    "requires an argument"

# Test: --max-date missing argument
check_error "convert: --max-date missing argument" \
    "echo '{}' | ./sensor-data convert --max-date" \
    "requires an argument"

# Test: -o missing argument
check_error "convert: -o missing argument" \
    "echo '{}' | ./sensor-data convert -o" \
    "requires an argument"

# Test: --output missing argument
check_error "convert: --output missing argument" \
    "echo '{}' | ./sensor-data convert --output" \
    "requires an argument"

# Test: -F missing argument
check_error "convert: -F missing argument" \
    "echo '{}' | ./sensor-data convert -F" \
    "requires an argument"

# Test: --output-format missing argument
check_error "convert: --output-format missing argument" \
    "echo '{}' | ./sensor-data convert --output-format" \
    "requires an argument"

# Test: --not-empty missing argument
check_error "convert: --not-empty missing argument" \
    "echo '{}' | ./sensor-data convert --not-empty" \
    "requires an argument"

# Test: --only-value missing argument
check_error "convert: --only-value missing argument" \
    "echo '{}' | ./sensor-data convert --only-value" \
    "requires an argument"

# Test: -f missing argument
check_error "convert: -f missing argument" \
    "echo '{}' | ./sensor-data convert -f" \
    "requires an argument"

# Test: --format missing argument
check_error "convert: --format missing argument" \
    "echo '{}' | ./sensor-data convert --format" \
    "requires an argument"

# Test: -e missing argument
check_error "convert: -e missing argument" \
    "echo '{}' | ./sensor-data convert -e" \
    "requires an argument"

# Test: -d missing argument
check_error "convert: -d missing argument" \
    "echo '{}' | ./sensor-data convert -d" \
    "requires an argument"

# Test: stats -c missing argument
check_error "stats: -c missing argument" \
    "echo '{}' | ./sensor-data stats -c" \
    "requires an argument"

# Test: stats --column missing argument
check_error "stats: --column missing argument" \
    "echo '{}' | ./sensor-data stats --column" \
    "requires an argument"

# ============================================
# INVALID FORMAT ERRORS
# ============================================

echo ""
echo "--- Invalid format errors ---"

# Test: Invalid --output-format value
check_error "convert: Invalid --output-format value" \
    "echo '{}' | ./sensor-data convert -F xml" \
    "must be 'json' or 'csv'"

# Test: Invalid --output-format value (yaml)
check_error "convert: Invalid --output-format value (yaml)" \
    "echo '{}' | ./sensor-data convert --output-format yaml" \
    "must be 'json' or 'csv'"

# Test: Invalid --only-value format (no colon)
check_error "convert: --only-value missing colon" \
    "echo '{}' | ./sensor-data convert --only-value sensor_value" \
    "requires format 'column:value'"

# Test: Invalid --only-value format (colon at start)
check_error "convert: --only-value colon at start" \
    "echo '{}' | ./sensor-data convert --only-value :value" \
    "requires format 'column:value'"

# Test: Invalid --only-value format (colon at end)
check_error "convert: --only-value colon at end" \
    "echo '{}' | ./sensor-data convert --only-value column:" \
    "requires format 'column:value'"

# Test: Invalid depth value (non-numeric)
check_error "convert: Invalid depth value (non-numeric)" \
    "echo '{}' | ./sensor-data convert -d abc" \
    "invalid depth value"

# Test: Invalid depth value (negative)
check_error "convert: Invalid depth value (negative)" \
    "echo '{}' | ./sensor-data convert -d -5" \
    "depth must be non-negative"

# ============================================
# UNKNOWN OPTION ERRORS
# ============================================

echo ""
echo "--- Unknown option errors ---"

# Test: Unknown option in stats
check_error "stats: Unknown option" \
    "echo '{}' | ./sensor-data stats --unknown-option" \
    "Unknown option"

# Test: Unknown option in list-errors
check_error "list-errors: Unknown option" \
    "echo '{}' | ./sensor-data list-errors --invalid-flag" \
    "Unknown option"

# Test: Unknown option in summarise-errors
check_error "summarise-errors: Unknown option" \
    "echo '{}' | ./sensor-data summarise-errors --bad-option" \
    "Unknown option"

# Test: Unknown command
check_error "Unknown command" \
    "./sensor-data unknown-command" \
    "Unknown command"

# ============================================
# NO INPUT DATA ERRORS
# ============================================

echo ""
echo "--- No input data errors ---"

# Test: Empty stdin for convert with streaming JSON
check_error "convert: Empty stdin" \
    "echo '' | ./sensor-data convert" \
    "No input"

# ============================================
# FILE/DIRECTORY ERRORS
# ============================================

echo ""
echo "--- File/directory errors ---"

# Test: Non-existent file
check_error "convert: Non-existent file" \
    "./sensor-data convert nonexistent_file.json 2>&1" \
    "Cannot open\|No such file\|Warning"

# Test: Non-existent directory
check_error "convert: Non-existent directory" \
    "./sensor-data convert /nonexistent/path/ 2>&1" \
    "Cannot open\|No such\|Warning\|No input"

# ============================================
# VALID EDGE CASES (should NOT error)
# ============================================

echo ""
echo "--- Valid edge cases (should succeed) ---"

# Test: Valid date format YYYY-MM-DD
echo ""
echo "Test: Valid date format YYYY-MM-DD"
result=$(echo '{"sensor":"ds18b20","value":"22.5","timestamp":"2024-01-15"}' | ./sensor-data convert --min-date 2024-01-01 --max-date 2024-12-31 2>&1) || true
if ! echo "$result" | grep -qi "error"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Should not produce error"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Valid date format with time
echo ""
echo "Test: Valid date format with time"
result=$(echo '{"sensor":"ds18b20","value":"22.5","timestamp":"2024-01-15T10:00:00"}' | ./sensor-data convert --min-date 2024-01-15T00:00:00 --max-date 2024-01-15T23:59:59 2>&1) || true
if ! echo "$result" | grep -qi "error"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Should not produce error"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Valid --only-value format
echo ""
echo "Test: Valid --only-value format"
result=$(echo '{"sensor":"ds18b20","value":"22.5"}' | ./sensor-data convert --only-value sensor:ds18b20 2>&1) || true
if ! echo "$result" | grep -qi "error"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Should not produce error"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# Test: Valid depth value
echo ""
echo "Test: Valid depth value"
result=$(echo '{"sensor":"ds18b20","value":"22.5"}' | ./sensor-data convert -d 0 2>&1) || true
if ! echo "$result" | grep -qi "error.*depth"; then
    echo "  ✓ PASS"
    PASSED=$((PASSED + 1))
else
    echo "  ✗ FAIL - Should not produce depth error"
    echo "  Got: $result"
    FAILED=$((FAILED + 1))
fi

# ============================================
# SUMMARY
# ============================================

echo ""
echo "================================"
echo "Error Handling Test Summary"
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
