CXX ?= g++
CC = gcc
CXXFLAGS ?= -Wall
CFLAGS ?= -Wall -Wextra -pedantic
# Always add optimization - O3 for maximum performance
CXXFLAGS += -O3 -std=c++11 -pthread -Iinclude
CFLAGS += -O3 -Iinclude
LDFLAGS += -pthread
CPPFLAGS ?=
PREFIX = /usr
BINDIR = $(PREFIX)/bin

# Detect OS for ncurses/pdcurses
# Check for MSYSTEM (set in MSYS2 environment) or Windows_NT
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)
MSYSTEM_VAL := $(shell echo $$MSYSTEM)
ifneq (,$(findstring MINGW,$(MSYSTEM_VAL)))
    # MSYS2 MinGW environment - PDCurses headers are in pdcurses subdirectory
    # PDC_WIDE and PDC_FORCE_UTF8 must match how the library was built
    CFLAGS += -I/mingw64/include/pdcurses -DPDC_WIDE -DPDC_FORCE_UTF8
    LDFLAGS_NCURSES = -L/mingw64/lib -lpdcurses
    TARGET_MON_EXT = .exe
    TARGET_EXT = .exe
else ifneq (,$(findstring MINGW,$(UNAME_S)))
    # MSYS2 MinGW detected via uname
    CFLAGS += -I/mingw64/include/pdcurses -DPDC_WIDE -DPDC_FORCE_UTF8
    LDFLAGS_NCURSES = -L/mingw64/lib -lpdcurses
    TARGET_MON_EXT = .exe
    TARGET_EXT = .exe
else ifeq ($(OS),Windows_NT)
    # Windows native
    CFLAGS += -I/mingw64/include/pdcurses -DPDC_WIDE -DPDC_FORCE_UTF8
    LDFLAGS_NCURSES = -L/mingw64/lib -lpdcurses
    TARGET_MON_EXT = .exe
    TARGET_EXT = .exe
else ifeq ($(UNAME_S),Darwin)
    # macOS
    LDFLAGS_NCURSES = -lncurses
    TARGET_MON_EXT =
    TARGET_EXT =
else
    # Linux and other POSIX
    LDFLAGS_NCURSES = -lncurses
    TARGET_MON_EXT =
    TARGET_EXT =
endif

# Extract version: try dpkg-parsechangelog, then git tag, else "unknown"
# Note: debian/changelog parsing removed due to shell escaping issues in Make
VERSION := $(shell dpkg-parsechangelog -S Version 2>/dev/null || git describe --tags --abbrev=0 2>/dev/null || echo unknown)
CPPFLAGS += -DVERSION='"$(VERSION)"'

# Coverage flags (set COVERAGE=1 to enable)
ifdef COVERAGE
CXXFLAGS += --coverage
CFLAGS += --coverage
LDFLAGS += --coverage
endif

# Source files for sensor-data (C++)
SOURCES = src/sensor-data.cpp
LIB_SOURCES = src/csv_parser.cpp src/json_parser.cpp src/error_detector.cpp src/file_utils.cpp src/sensor_data_transformer.cpp src/data_counter.cpp src/error_lister.cpp src/error_summarizer.cpp src/stats_analyser.cpp src/latest_finder.cpp src/sensor_data_api.cpp src/rdata_writer.cpp
TEST_SOURCES = tests/test_csv_parser.cpp tests/test_json_parser.cpp tests/test_error_detector.cpp tests/test_file_utils.cpp tests/test_date_utils.cpp tests/test_common_arg_parser.cpp tests/test_data_reader.cpp tests/test_file_collector.cpp tests/test_command_base.cpp tests/test_stats_analyser.cpp tests/test_rdata_writer.cpp

# Source files for sensor-mon (C)
MON_SOURCES = src/sensor-mon.c src/graph.c

# Source files for sensor-plot (C)
PLOT_SOURCES = src/sensor-plot.c src/graph.c

# Object files
LIB_OBJECTS = $(LIB_SOURCES:.cpp=.o)
MON_OBJECTS = $(MON_SOURCES:.c=.o)
PLOT_OBJECTS = src/sensor-plot.o src/graph.o src/sensor_plot_args.o
TEST_EXECUTABLES = test_csv_parser test_json_parser test_error_detector test_file_utils test_date_utils test_common_arg_parser test_data_reader test_file_collector test_command_base test_stats_analyser test_graph test_sensor_plot_args test_rdata_writer

TARGET = sensor-data
TARGET_MON = sensor-mon
TARGET_PLOT = sensor-plot

all: $(TARGET) $(TARGET_MON) $(TARGET_PLOT)

# Build library objects
%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Build C objects for sensor-mon
src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Build main application (sensor-data)
$(TARGET): $(LIB_OBJECTS) $(SOURCES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LIB_OBJECTS) $(LDFLAGS)

# Build sensor-mon (links with C++ API objects, so uses C++ linker)
API_OBJECTS = src/sensor_data_api.o src/csv_parser.o src/json_parser.o src/file_utils.o src/error_detector.o
$(TARGET_MON): $(MON_OBJECTS) $(API_OBJECTS)
	$(CXX) $(CFLAGS) -o $(TARGET_MON) $(MON_OBJECTS) $(API_OBJECTS) $(LDFLAGS) $(LDFLAGS_NCURSES)

# Build sensor-plot (links with C++ API objects, so uses C++ linker)
$(TARGET_PLOT): $(PLOT_OBJECTS) $(API_OBJECTS)
	$(CXX) $(CFLAGS) -o $(TARGET_PLOT) $(PLOT_OBJECTS) $(API_OBJECTS) $(LDFLAGS) $(LDFLAGS_NCURSES)

# Build and run tests
test: $(LIB_OBJECTS)
	@echo "Building and running unit tests..."
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_csv_parser.cpp src/csv_parser.o -o test_csv_parser $(LDFLAGS) && ./test_csv_parser
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_json_parser.cpp src/json_parser.o -o test_json_parser $(LDFLAGS) && ./test_json_parser
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_error_detector.cpp src/error_detector.o -o test_error_detector $(LDFLAGS) && ./test_error_detector
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_file_utils.cpp src/file_utils.o -o test_file_utils $(LDFLAGS) && ./test_file_utils
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_date_utils.cpp -o test_date_utils $(LDFLAGS) && ./test_date_utils
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_common_arg_parser.cpp src/file_utils.o -o test_common_arg_parser $(LDFLAGS) && ./test_common_arg_parser
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_data_reader.cpp src/csv_parser.o src/json_parser.o src/file_utils.o src/error_detector.o -o test_data_reader $(LDFLAGS) && ./test_data_reader
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_file_collector.cpp src/file_utils.o -o test_file_collector $(LDFLAGS) && ./test_file_collector
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_command_base.cpp src/csv_parser.o src/json_parser.o src/file_utils.o src/error_detector.o -o test_command_base $(LDFLAGS) && ./test_command_base
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_stats_analyser.cpp -o test_stats_analyser $(LDFLAGS) && ./test_stats_analyser
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c src/graph.c -o src/graph.o
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_graph.cpp src/graph.o -o test_graph $(LDFLAGS) $(LDFLAGS_NCURSES) && ./test_graph
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c src/sensor_plot_args.c -o src/sensor_plot_args.o
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_sensor_plot_args.cpp src/sensor_plot_args.o -o test_sensor_plot_args $(LDFLAGS) && ./test_sensor_plot_args
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/test_rdata_writer.cpp src/rdata_writer.o -o test_rdata_writer $(LDFLAGS) && ./test_rdata_writer
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

install: $(TARGET) $(TARGET_MON) $(TARGET_PLOT)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/
	install -m 755 $(TARGET_MON) $(DESTDIR)$(BINDIR)/
	install -m 755 $(TARGET_PLOT) $(DESTDIR)$(BINDIR)/
	install -d $(DESTDIR)/etc/ws/sensor-errors
	install -m 644 etc/ws/sensor-errors/*.errors $(DESTDIR)/etc/ws/sensor-errors/
	install -d $(DESTDIR)/usr/share/bash-completion/completions
	install -m 644 completions/sensor-data.bash $(DESTDIR)/usr/share/bash-completion/completions/sensor-data

clean:
	rm -f $(TARGET) $(TARGET_MON) $(TARGET_PLOT) $(LIB_OBJECTS) $(MON_OBJECTS) $(PLOT_OBJECTS) $(TEST_EXECUTABLES) src/*.o *.gcda *.gcno src/*.gcda src/*.gcno

# Force a clean rebuild
rebuild: clean all

# Release build with maximum optimizations
release: clean
	$(CXX) $(CPPFLAGS) -std=c++11 -pthread -Iinclude -O3 -march=native -flto -DNDEBUG -o $(TARGET) $(SOURCES) $(LIB_SOURCES) -pthread
	$(CXX) $(CPPFLAGS) -std=c++11 -Iinclude -O3 -march=native -flto -DNDEBUG -o $(TARGET_MON) $(MON_SOURCES) src/sensor_data_api.cpp src/csv_parser.cpp src/json_parser.cpp src/file_utils.cpp src/error_detector.cpp $(LDFLAGS_NCURSES)
	$(CXX) $(CPPFLAGS) -std=c++11 -Iinclude -O3 -march=native -flto -DNDEBUG -o $(TARGET_PLOT) $(PLOT_SOURCES) src/sensor_data_api.cpp src/csv_parser.cpp src/json_parser.cpp src/file_utils.cpp src/error_detector.cpp $(LDFLAGS_NCURSES)

# Generate coverage report (requires gcov)
coverage:
	@echo "=== Coverage files ==="
	@ls -la *.gcno *.gcda 2>/dev/null || true
	@ls -la src/*.gcno src/*.gcda 2>/dev/null || true
	@echo "=== Running gcov ==="
	gcov src/*.cpp tests/*.cpp 2>/dev/null || true
	@echo "=== Generated .gcov files ==="
	@ls -la *.gcov 2>/dev/null || true

.PHONY: all install clean rebuild test test-integration test-all coverage release

