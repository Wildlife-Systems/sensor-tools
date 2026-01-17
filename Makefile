CXX = g++
CXXFLAGS = -Wall -O2 -std=c++11 -pthread -Iinclude
LDFLAGS = -pthread
PREFIX = /usr
BINDIR = $(PREFIX)/bin

# Source files
SOURCES = src/sensor-data.cpp
LIB_SOURCES = src/csv_parser.cpp src/json_parser.cpp src/error_detector.cpp src/file_utils.cpp
TEST_SOURCES = tests/test_csv_parser.cpp tests/test_json_parser.cpp tests/test_error_detector.cpp tests/test_file_utils.cpp

# Object files
LIB_OBJECTS = $(LIB_SOURCES:.cpp=.o)
TEST_EXECUTABLES = test_csv_parser test_json_parser test_error_detector test_file_utils

TARGET = sensor-data

all: $(TARGET)

# Build library objects
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build main application
$(TARGET): $(LIB_OBJECTS) $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

# Build and run tests
test: $(LIB_OBJECTS)
	@echo "Building and running tests..."
	@$(CXX) $(CXXFLAGS) tests/test_csv_parser.cpp src/csv_parser.o -o test_csv_parser && ./test_csv_parser
	@$(CXX) $(CXXFLAGS) tests/test_json_parser.cpp src/json_parser.o -o test_json_parser && ./test_json_parser
	@$(CXX) $(CXXFLAGS) tests/test_error_detector.cpp src/error_detector.o -o test_error_detector && ./test_error_detector
	@$(CXX) $(CXXFLAGS) tests/test_file_utils.cpp src/file_utils.o -o test_file_utils && ./test_file_utils
	@echo "All tests passed!"

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/

clean:
	rm -f $(TARGET) $(LIB_OBJECTS) $(TEST_EXECUTABLES) src/*.o

.PHONY: all install clean test

