CXX = g++
CXXFLAGS = -Wall -O2 -std=c++11 -pthread -Iinclude
LDFLAGS = -pthread
PREFIX = /usr
BINDIR = $(PREFIX)/bin

# Coverage flags (set COVERAGE=1 to enable)
ifdef COVERAGE
CXXFLAGS += --coverage
LDFLAGS += --coverage
endif

# Source files
SOURCES = src/sensor-data.cpp
LIB_SOURCES = src/csv_parser.cpp src/json_parser.cpp src/error_detector.cpp src/file_utils.cpp
TEST_SOURCES = tests/test_csv_parser.cpp tests/test_json_parser.cpp tests/test_error_detector.cpp tests/test_file_utils.cpp tests/test_date_utils.cpp

# Object files
LIB_OBJECTS = $(LIB_SOURCES:.cpp=.o)
TEST_EXECUTABLES = test_csv_parser test_json_parser test_error_detector test_file_utils test_date_utils

TARGET = sensor-data

all: $(TARGET)

# Build library objects
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build main application
$(TARGET): $(LIB_OBJECTS) $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LIB_OBJECTS) $(LDFLAGS)

# Build and run tests
test: $(LIB_OBJECTS)
	@echo "Building and running unit tests..."
	@$(CXX) $(CXXFLAGS) tests/test_csv_parser.cpp src/csv_parser.o -o test_csv_parser && ./test_csv_parser
	@$(CXX) $(CXXFLAGS) tests/test_json_parser.cpp src/json_parser.o -o test_json_parser && ./test_json_parser
	@$(CXX) $(CXXFLAGS) tests/test_error_detector.cpp src/error_detector.o -o test_error_detector && ./test_error_detector
	@$(CXX) $(CXXFLAGS) tests/test_file_utils.cpp src/file_utils.o -o test_file_utils && ./test_file_utils
	@$(CXX) $(CXXFLAGS) tests/test_date_utils.cpp -o test_date_utils && ./test_date_utils
	@echo "All unit tests passed!"

# Run integration tests (requires bash)
test-integration: $(TARGET)
	@echo "Running integration tests..."
	@bash tests/test_convert.sh
	@bash tests/test_stdin_stdout.sh
	@bash tests/test_list_errors_stdin.sh
	@bash tests/test_summarise_errors.sh
	@bash tests/test_stats.sh
	@bash tests/test_csv_input.sh
	@echo "All integration tests passed!"

# Run all tests
test-all: test test-integration

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/

clean:
	rm -f $(TARGET) $(LIB_OBJECTS) $(TEST_EXECUTABLES) src/*.o *.gcda *.gcno src/*.gcda src/*.gcno

# Generate coverage report (requires gcov)
coverage:
	@echo "=== Coverage files ==="
	@ls -la *.gcno *.gcda 2>/dev/null || true
	@ls -la src/*.gcno src/*.gcda 2>/dev/null || true
	@echo "=== Running gcov ==="
	gcov src/*.cpp tests/*.cpp 2>/dev/null || true
	@echo "=== Generated .gcov files ==="
	@ls -la *.gcov 2>/dev/null || true

.PHONY: all install clean test test-integration test-all coverage

