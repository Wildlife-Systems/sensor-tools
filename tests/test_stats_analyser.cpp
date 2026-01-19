/**
 * Unit tests for StatsAnalyser static calculation methods
 */

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

// We need to test private static methods, so we'll use a friend class workaround
// Define the test to access protected/private methods

// First, declare the class with public access to static methods for testing
class StatsAnalyser {
public:
    static bool isNumeric(const std::string& str);
    static double calculateMedian(const std::vector<double>& values);
    static double calculateStdDev(const std::vector<double>& values, double mean);
    static double calculatePercentile(const std::vector<double>& sortedValues, double percentile);
};

// Implementations copied from stats_analyser.cpp for isolated unit testing
bool StatsAnalyser::isNumeric(const std::string& str) {
    if (str.empty()) return false;
    try {
        size_t pos = 0;
        std::stod(str, &pos);
        return pos == str.length();
    } catch (...) {
        return false;
    }
}

double StatsAnalyser::calculateMedian(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    size_t n = sorted.size();
    if (n % 2 == 0) {
        return (sorted[n/2 - 1] + sorted[n/2]) / 2.0;
    } else {
        return sorted[n/2];
    }
}

double StatsAnalyser::calculateStdDev(const std::vector<double>& values, double mean) {
    if (values.size() <= 1) return 0.0;
    double sum = 0.0;
    for (double v : values) {
        sum += (v - mean) * (v - mean);
    }
    return std::sqrt(sum / (values.size() - 1));
}

double StatsAnalyser::calculatePercentile(const std::vector<double>& sortedValues, double percentile) {
    if (sortedValues.empty()) return 0.0;
    if (sortedValues.size() == 1) return sortedValues[0];
    
    double index = (percentile / 100.0) * (sortedValues.size() - 1);
    size_t lower = static_cast<size_t>(index);
    size_t upper = lower + 1;
    double fraction = index - lower;
    
    if (upper >= sortedValues.size()) {
        return sortedValues[sortedValues.size() - 1];
    }
    
    return sortedValues[lower] + fraction * (sortedValues[upper] - sortedValues[lower]);
}

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (!(condition)) { \
            std::cerr << "[FAIL] " << __func__ << ": " << message << std::endl; \
            return false; \
        } \
        tests_passed++; \
        return true; \
    } while(0)

#define ASSERT_TRUE(condition) ASSERT(condition, #condition " should be true")
#define ASSERT_FALSE(condition) ASSERT(!(condition), #condition " should be false")
#define ASSERT_EQ(a, b) ASSERT((a) == (b), #a " should equal " #b)
#define ASSERT_NEAR(a, b, eps) ASSERT(std::fabs((a) - (b)) < (eps), #a " should be near " #b)

// ==================== isNumeric tests ====================

bool test_isNumeric_integer() {
    ASSERT_TRUE(StatsAnalyser::isNumeric("123"));
}

bool test_isNumeric_negative() {
    ASSERT_TRUE(StatsAnalyser::isNumeric("-456"));
}

bool test_isNumeric_decimal() {
    ASSERT_TRUE(StatsAnalyser::isNumeric("25.5"));
}

bool test_isNumeric_negative_decimal() {
    ASSERT_TRUE(StatsAnalyser::isNumeric("-127.5"));
}

bool test_isNumeric_scientific() {
    ASSERT_TRUE(StatsAnalyser::isNumeric("1.5e10"));
}

bool test_isNumeric_zero() {
    ASSERT_TRUE(StatsAnalyser::isNumeric("0"));
}

bool test_isNumeric_empty() {
    ASSERT_FALSE(StatsAnalyser::isNumeric(""));
}

bool test_isNumeric_text() {
    ASSERT_FALSE(StatsAnalyser::isNumeric("hello"));
}

bool test_isNumeric_mixed() {
    ASSERT_FALSE(StatsAnalyser::isNumeric("123abc"));
}

bool test_isNumeric_spaces() {
    ASSERT_FALSE(StatsAnalyser::isNumeric("123 "));
}

bool test_isNumeric_leading_spaces() {
    // stod handles leading spaces but pos check catches trailing content
    // " 123" -> stod parses as 123, pos = 4, length = 4, so it passes
    ASSERT_TRUE(StatsAnalyser::isNumeric(" 123"));
}

// ==================== calculateMedian tests ====================

bool test_median_empty() {
    std::vector<double> values;
    ASSERT_NEAR(StatsAnalyser::calculateMedian(values), 0.0, 0.001);
}

bool test_median_single() {
    std::vector<double> values = {5.0};
    ASSERT_NEAR(StatsAnalyser::calculateMedian(values), 5.0, 0.001);
}

bool test_median_two_values() {
    std::vector<double> values = {10.0, 20.0};
    ASSERT_NEAR(StatsAnalyser::calculateMedian(values), 15.0, 0.001);
}

bool test_median_odd_count() {
    std::vector<double> values = {1.0, 3.0, 5.0, 7.0, 9.0};
    ASSERT_NEAR(StatsAnalyser::calculateMedian(values), 5.0, 0.001);
}

bool test_median_even_count() {
    std::vector<double> values = {1.0, 2.0, 3.0, 4.0};
    ASSERT_NEAR(StatsAnalyser::calculateMedian(values), 2.5, 0.001);
}

bool test_median_unsorted() {
    std::vector<double> values = {9.0, 1.0, 5.0, 7.0, 3.0};
    ASSERT_NEAR(StatsAnalyser::calculateMedian(values), 5.0, 0.001);
}

bool test_median_with_negatives() {
    std::vector<double> values = {-10.0, -5.0, 0.0, 5.0, 10.0};
    ASSERT_NEAR(StatsAnalyser::calculateMedian(values), 0.0, 0.001);
}

// ==================== calculateStdDev tests ====================

bool test_stddev_empty() {
    std::vector<double> values;
    ASSERT_NEAR(StatsAnalyser::calculateStdDev(values, 0.0), 0.0, 0.001);
}

bool test_stddev_single() {
    std::vector<double> values = {5.0};
    ASSERT_NEAR(StatsAnalyser::calculateStdDev(values, 5.0), 0.0, 0.001);
}

bool test_stddev_identical_values() {
    std::vector<double> values = {10.0, 10.0, 10.0, 10.0};
    ASSERT_NEAR(StatsAnalyser::calculateStdDev(values, 10.0), 0.0, 0.001);
}

bool test_stddev_simple() {
    // Values: 2, 4, 4, 4, 5, 5, 7, 9
    // Mean: 5
    // Variance: sum((v-5)^2) / (n-1) = (9+1+1+1+0+0+4+16)/7 = 32/7 ≈ 4.571
    // StdDev: sqrt(4.571) ≈ 2.138
    std::vector<double> values = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    double mean = 5.0;
    ASSERT_NEAR(StatsAnalyser::calculateStdDev(values, mean), 2.138, 0.01);
}

bool test_stddev_two_values() {
    // Values: 10, 20
    // Mean: 15
    // Variance: ((10-15)^2 + (20-15)^2) / 1 = 50
    // StdDev: sqrt(50) ≈ 7.071
    std::vector<double> values = {10.0, 20.0};
    ASSERT_NEAR(StatsAnalyser::calculateStdDev(values, 15.0), 7.071, 0.01);
}

// ==================== calculatePercentile tests ====================

bool test_percentile_empty() {
    std::vector<double> values;
    ASSERT_NEAR(StatsAnalyser::calculatePercentile(values, 50), 0.0, 0.001);
}

bool test_percentile_single() {
    std::vector<double> values = {42.0};
    ASSERT_NEAR(StatsAnalyser::calculatePercentile(values, 50), 42.0, 0.001);
}

bool test_percentile_0() {
    std::vector<double> values = {1.0, 2.0, 3.0, 4.0, 5.0};
    ASSERT_NEAR(StatsAnalyser::calculatePercentile(values, 0), 1.0, 0.001);
}

bool test_percentile_100() {
    std::vector<double> values = {1.0, 2.0, 3.0, 4.0, 5.0};
    ASSERT_NEAR(StatsAnalyser::calculatePercentile(values, 100), 5.0, 0.001);
}

bool test_percentile_50() {
    std::vector<double> values = {1.0, 2.0, 3.0, 4.0, 5.0};
    ASSERT_NEAR(StatsAnalyser::calculatePercentile(values, 50), 3.0, 0.001);
}

bool test_percentile_25() {
    std::vector<double> values = {1.0, 2.0, 3.0, 4.0, 5.0};
    // 0.25 * 4 = 1.0, so index 1 = value 2.0
    ASSERT_NEAR(StatsAnalyser::calculatePercentile(values, 25), 2.0, 0.001);
}

bool test_percentile_75() {
    std::vector<double> values = {1.0, 2.0, 3.0, 4.0, 5.0};
    // 0.75 * 4 = 3.0, so index 3 = value 4.0
    ASSERT_NEAR(StatsAnalyser::calculatePercentile(values, 75), 4.0, 0.001);
}

bool test_percentile_interpolation() {
    std::vector<double> values = {10.0, 20.0, 30.0, 40.0};
    // 0.50 * 3 = 1.5, interpolate between values[1]=20 and values[2]=30
    // 20 + 0.5*(30-20) = 25
    ASSERT_NEAR(StatsAnalyser::calculatePercentile(values, 50), 25.0, 0.001);
}

bool test_percentile_90() {
    std::vector<double> values = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
    // 0.90 * 9 = 8.1, interpolate between values[8]=9 and values[9]=10
    // 9 + 0.1*(10-9) = 9.1
    ASSERT_NEAR(StatsAnalyser::calculatePercentile(values, 90), 9.1, 0.001);
}

// ==================== Main ====================

int main() {
    std::cout << "Running StatsAnalyser unit tests..." << std::endl;
    
    // isNumeric tests
    std::cout << (test_isNumeric_integer() ? "[PASS]" : "[FAIL]") << " test_isNumeric_integer" << std::endl;
    std::cout << (test_isNumeric_negative() ? "[PASS]" : "[FAIL]") << " test_isNumeric_negative" << std::endl;
    std::cout << (test_isNumeric_decimal() ? "[PASS]" : "[FAIL]") << " test_isNumeric_decimal" << std::endl;
    std::cout << (test_isNumeric_negative_decimal() ? "[PASS]" : "[FAIL]") << " test_isNumeric_negative_decimal" << std::endl;
    std::cout << (test_isNumeric_scientific() ? "[PASS]" : "[FAIL]") << " test_isNumeric_scientific" << std::endl;
    std::cout << (test_isNumeric_zero() ? "[PASS]" : "[FAIL]") << " test_isNumeric_zero" << std::endl;
    std::cout << (test_isNumeric_empty() ? "[PASS]" : "[FAIL]") << " test_isNumeric_empty" << std::endl;
    std::cout << (test_isNumeric_text() ? "[PASS]" : "[FAIL]") << " test_isNumeric_text" << std::endl;
    std::cout << (test_isNumeric_mixed() ? "[PASS]" : "[FAIL]") << " test_isNumeric_mixed" << std::endl;
    std::cout << (test_isNumeric_spaces() ? "[PASS]" : "[FAIL]") << " test_isNumeric_spaces" << std::endl;
    std::cout << (test_isNumeric_leading_spaces() ? "[PASS]" : "[FAIL]") << " test_isNumeric_leading_spaces" << std::endl;
    
    // calculateMedian tests
    std::cout << (test_median_empty() ? "[PASS]" : "[FAIL]") << " test_median_empty" << std::endl;
    std::cout << (test_median_single() ? "[PASS]" : "[FAIL]") << " test_median_single" << std::endl;
    std::cout << (test_median_two_values() ? "[PASS]" : "[FAIL]") << " test_median_two_values" << std::endl;
    std::cout << (test_median_odd_count() ? "[PASS]" : "[FAIL]") << " test_median_odd_count" << std::endl;
    std::cout << (test_median_even_count() ? "[PASS]" : "[FAIL]") << " test_median_even_count" << std::endl;
    std::cout << (test_median_unsorted() ? "[PASS]" : "[FAIL]") << " test_median_unsorted" << std::endl;
    std::cout << (test_median_with_negatives() ? "[PASS]" : "[FAIL]") << " test_median_with_negatives" << std::endl;
    
    // calculateStdDev tests
    std::cout << (test_stddev_empty() ? "[PASS]" : "[FAIL]") << " test_stddev_empty" << std::endl;
    std::cout << (test_stddev_single() ? "[PASS]" : "[FAIL]") << " test_stddev_single" << std::endl;
    std::cout << (test_stddev_identical_values() ? "[PASS]" : "[FAIL]") << " test_stddev_identical_values" << std::endl;
    std::cout << (test_stddev_simple() ? "[PASS]" : "[FAIL]") << " test_stddev_simple" << std::endl;
    std::cout << (test_stddev_two_values() ? "[PASS]" : "[FAIL]") << " test_stddev_two_values" << std::endl;
    
    // calculatePercentile tests
    std::cout << (test_percentile_empty() ? "[PASS]" : "[FAIL]") << " test_percentile_empty" << std::endl;
    std::cout << (test_percentile_single() ? "[PASS]" : "[FAIL]") << " test_percentile_single" << std::endl;
    std::cout << (test_percentile_0() ? "[PASS]" : "[FAIL]") << " test_percentile_0" << std::endl;
    std::cout << (test_percentile_100() ? "[PASS]" : "[FAIL]") << " test_percentile_100" << std::endl;
    std::cout << (test_percentile_50() ? "[PASS]" : "[FAIL]") << " test_percentile_50" << std::endl;
    std::cout << (test_percentile_25() ? "[PASS]" : "[FAIL]") << " test_percentile_25" << std::endl;
    std::cout << (test_percentile_75() ? "[PASS]" : "[FAIL]") << " test_percentile_75" << std::endl;
    std::cout << (test_percentile_interpolation() ? "[PASS]" : "[FAIL]") << " test_percentile_interpolation" << std::endl;
    std::cout << (test_percentile_90() ? "[PASS]" : "[FAIL]") << " test_percentile_90" << std::endl;
    
    std::cout << "\n" << tests_passed << "/" << tests_run << " tests passed" << std::endl;
    
    return tests_passed == tests_run ? 0 : 1;
}
