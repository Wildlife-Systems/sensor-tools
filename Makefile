CXX ?= g++
CXXFLAGS ?= -Wall -O2
CXXFLAGS += -std=c++11 -pthread -Iinclude
LDFLAGS += -pthread
CPPFLAGS ?=
PREFIX = /usr
BINDIR = $(PREFIX)/bin

# Extract version from debian/changelog
VERSION := $(shell dpkg-parsechangelog -S Version 2>/dev/null || echo "unknown")
CPPFLAGS += -DVERSION='"$(VERSION)"'

# Coverage flags (set COVERAGE=1 to enable)
ifdef COVERAGE
CXXFLAGS += --coverage
LDFLAGS += --coverage
endif

# Source files
SOURCES = src/sensor-data.cpp
LIB_SOURCES = src/csv_parser.cpp src/json_parser.cpp src/error_detector.cpp src/file_utils.cpp src/sensor_data_transformer.cpp src/data_counter.cpp src/error_lister.cpp src/error_summarizer.cpp src/stats_analyser.cpp src/latest_finder.cpp src/data_reader.cpp
TEST_SOURCES = tests/test_csv_parser.cpp tests/test_json_parser.cpp tests/test_error_detector.cpp tests/test_file_utils.cpp tests/test_date_utils.cpp tests/test_common_arg_parser.cpp tests/test_data_reader.cpp tests/test_file_collector.cpp tests/test_command_base.cpp tests/test_stats_analyser.cpp

# Object files
LIB_OBJECTS = $(LIB_SOURCES:.cpp=.o)
TEST_EXECUTABLES = test_csv_parser test_json_parser test_error_detector test_file_utils test_date_utils test_common_arg_parser test_data_reader test_file_collector test_command_base test_stats_analyser

TARGET = sensor-data

all: $(TARGET)

# Build library objects
%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Build main application
$(TARGET): $(LIB_OBJECTS) $(SOURCES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LIB_OBJECTS) $(LDFLAGS)

# Build and run tests
test: $(LIB_OBJECTS)
	@echo "Building and running unit tests..."
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_csv_parser.cpp src/csv_parser.o -o test_csv_parser $(LDFLAGS) && ./test_csv_parser
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_json_parser.cpp src/json_parser.o -o test_json_parser $(LDFLAGS) && ./test_json_parser
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_error_detector.cpp src/error_detector.o -o test_error_detector $(LDFLAGS) && ./test_error_detector
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_file_utils.cpp src/file_utils.o -o test_file_utils $(LDFLAGS) && ./test_file_utils
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_date_utils.cpp -o test_date_utils $(LDFLAGS) && ./test_date_utils
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_common_arg_parser.cpp src/file_utils.o -o test_common_arg_parser $(LDFLAGS) && ./test_common_arg_parser
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_data_reader.cpp src/csv_parser.o src/json_parser.o src/file_utils.o -o test_data_reader $(LDFLAGS) && ./test_data_reader
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_file_collector.cpp src/file_utils.o -o test_file_collector $(LDFLAGS) && ./test_file_collector
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_command_base.cpp src/error_detector.o -o test_command_base $(LDFLAGS) && ./test_command_base
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_stats_analyser.cpp -o test_stats_analyser $(LDFLAGS) && ./test_stats_analyser
	@echo "All unit tests passed!"

# Run integration tests (requires bash)
test-integration: $(TARGET)
	@echo "Running integration tests..."
	@bash tests/test_transform.sh
	@bash tests/test_count.sh
	@bash tests/test_stdin_stdout.sh
	@bash tests/test_list_errors_stdin.sh
	@bash tests/test_summarise_errors.sh
	@bash tests/test_stats.sh
	@bash tests/test_csv_input.sh
	@bash tests/test_error_handling.sh
	@bash tests/test_latest.sh
	@echo "All integration tests passed!"

# Run all tests
test-all: test test-integration

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/
	install -d $(DESTDIR)/etc/ws/sensor-errors
	install -m 644 etc/ws/sensor-errors/*.errors $(DESTDIR)/etc/ws/sensor-errors/
	install -d $(DESTDIR)/usr/share/bash-completion/completions
	install -m 644 completions/sensor-data.bash $(DESTDIR)/usr/share/bash-completion/completions/sensor-data

clean:
	rm -f $(TARGET) $(LIB_OBJECTS) $(TEST_EXECUTABLES) src/*.o *.gcda *.gcno src/*.gcda src/*.gcno

# Force a clean rebuild
rebuild: clean all

# Generate coverage report (requires gcov)
coverage:
	@echo "=== Coverage files ==="
	@ls -la *.gcno *.gcda 2>/dev/null || true
	@ls -la src/*.gcno src/*.gcda 2>/dev/null || true
	@echo "=== Running gcov ==="
	gcov src/*.cpp tests/*.cpp 2>/dev/null || true
	@echo "=== Generated .gcov files ==="
	@ls -la *.gcov 2>/dev/null || true

.PHONY: all install clean rebuild test test-integration test-all coverage

