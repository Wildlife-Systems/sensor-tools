# Sensor Tools

A C++ library and command-line tool for managing and reading sensor data.

## Features

- Convert JSON and CSV sensor data files to unified CSV format
- Detect and filter sensor error readings (DS18B20 temperature errors)
- Modular architecture with separate parser components
- Support for recursive directory processing
- Comprehensive test suite with CI/CD integration
- Cross-platform support (Linux, Windows, macOS)

## Building from Source

### Prerequisites

**Linux:**
```bash
sudo apt-get install build-essential g++
```

**macOS:**
```bash
brew install gcc
```

**Windows:**
Install MinGW-w64 or similar GCC compiler

### Build

Using Make:
```bash
make
```

Or manually:
```bash
g++ -std=c++11 -pthread -Iinclude src/sensor-data.cpp -o sensor-data
```

### Running Tests

**Linux:**
```bash
make test
# or
cd tests && ./run_tests.sh
```

The test suite includes:
- Unit tests for CSV parser
- Unit tests for JSON parser
- Unit tests for error detection
- Unit tests for file utilities
- Integration tests for the complete application

### Install

```bash
sudo make install
```

## Continuous Integration

The project uses GitHub Actions for automated testing on every push and pull request. Tests run on Ubuntu Linux and include:
- Building all components
- Running unit tests
- Running integration tests
- Verifying error detection and filtering

## Building Debian Package

To build the Debian package:

```bash
dpkg-buildpackage -us -uc -b
```

This will create three packages in the parent directory:
- `sensor-tools_1.0.0-1_<arch>.deb` - Main executable
- `libsensortools1_1.0.0-1_<arch>.deb` - Shared library
- `libsensortools-dev_1.0.0-1_<arch>.deb` - Development headers

### Installing the Package

```bash
sudo dpkg -i ../sensor-tools_1.0.0-1_*.deb
sudo dpkg -i ../libsensortools1_1.0.0-1_*.deb
```

## Usage

### sensor-data

Convert JSON or CSV sensor data files to CSV:

#### Input Format Detection

The tool automatically detects the input file format based on file extension:
- **CSV files** (`.csv` extension): Processed as standard CSV with header row
- **JSON files** (all other extensions): Each line should contain a JSON object with sensor readings

Both formats can be mixed in the same conversion operation, and all data will be merged into a single unified CSV output.

#### Examples

```bash
# Single file
sensor-data convert input.out output.csv

# CSV input file
sensor-data convert input.csv output.csv

# Multiple files (mixed formats)
sensor-data convert sensor1.out sensor2.csv sensor3.out output.csv

# Multiple files
sensor-data convert sensor1.out sensor2.out sensor3.out output.csv

# Directory of files (non-recursive by default)
sensor-data convert /path/to/sensor/logs/ output.csv

# Directory with extension filter
sensor-data convert -e .out /path/to/sensor/logs/ output.csv

# Process only CSV files from directory
sensor-data convert -e .csv /path/to/sensor/logs/ output.csv

# Recursive directory traversal
sensor-data convert -r /path/to/sensor/logs/ output.csv

# Recursive with extension filter
sensor-data convert -r -e .out /path/to/sensor/logs/ output.csv

# Limit recursion depth to 2 levels
sensor-data convert -r -d 2 /path/to/sensor/logs/ output.csv

# Combined: recursive with depth limit and extension filter
sensor-data convert -r -d 2 -e .out /path/to/sensor/logs/ output.csv

# Use sc-prototype for column definitions (skips discovery pass)
sensor-data convert --use-prototype -r -e .out /path/to/sensor/logs/ output.csv

# Skip rows where specific columns are empty
sensor-data convert --not-empty unit --not-empty value -e .out /path/to/logs/ output.csv

# Verbose output showing progress
sensor-data convert -v -r -e .out /path/to/logs/ output.csv

# Very verbose output showing detailed file operations
sensor-data convert -V -r -e .out /path/to/logs/ output.csv

# Filter to only rows where type is "temperature"
sensor-data convert --only-value type:temperature -r -e .out /path/to/logs/ output.csv

# Multiple value filters (AND logic)
sensor-data convert --only-value type:temperature --only-value unit:C /path/to/logs/ output.csv

# Combined filters
sensor-data convert --not-empty value --only-value type:temperature -e .out /logs output.csv

# Remove error readings (DS18B20 temperature=85)
sensor-data convert --remove-errors sensor1.out output.csv

# Mixed input
sensor-data convert file1.out file2.out /path/to/dir/ output.csv
```

**Options:**
- `-r, --recursive` - Recursively process subdirectories
- `-v` - Verbose output (show file progress and configuration)
- `-V` - Very verbose output (show detailed progress including files found/skipped and row filtering)
- `-e, --extension <ext>` - Filter files by extension (e.g., `.out` or `out`)
- `-d, --depth <n>` - Maximum recursion depth (0 = current directory only, default = unlimited)
- `--use-prototype` - Use `sc-prototype` command to define columns (skips column discovery pass and filters to only prototype columns)
- `--not-empty <column>` - Skip rows where the specified column is missing or empty (can be used multiple times)
- `--only-value <column:value>` - Only include rows where the specified column has an exact value match (can be used multiple times for AND logic)
- `--remove-errors` - Remove error readings (currently detects DS18B20 sensors with temperature/value = 85)

**Using `--use-prototype`:**

When this flag is used, `sensor-data` executes the `sc-prototype` command to get column definitions instead of discovering them from input files. This:
- Skips the first pass (column discovery), making processing faster
- Only includes columns defined in the prototype
- Ignores any extra columns found in the data files

The `sc-prototype` command should output a single JSON line with all expected fields set to null values.

The `sensor-data` tool reads JSON-formatted or CSV-formatted sensor data files
and converts them to CSV format. 

For JSON input: Each line should contain a JSON object with sensor readings.
For CSV input: Files must have a header row defining column names.

Each sensor reading becomes a row in the output CSV. When processing multiple files with different columns, the tool merges all unique columns into the output.

#### Error Detection

The tool can detect and handle error readings from sensors:

**DS18B20 Temperature Sensors**: When a DS18B20 sensor returns a value of 85°C, this indicates an error condition (sensor initialization failure or communication error).

##### List Errors

Find all error readings in your data files:

```bash
# Single file
sensor-data list-errors sensor1.out

# Multiple files
sensor-data list-errors sensor1.out sensor2.csv

# Recursive directory scan
sensor-data list-errors -r -e .out /path/to/logs/

# With extension filter
sensor-data list-errors -e .out /path/to/logs/
```

**list-errors Options:**
- `-r, --recursive` - Recursively process subdirectories
- `-v` - Verbose output
- `-V` - Very verbose output
- `-e, --extension <ext>` - Filter files by extension
- `-d, --depth <n>` - Maximum recursion depth

The output shows the file, line number, and relevant sensor information for each error found:
```
test-errors.out:2 type=ds18b20 sensor_id=sensor002 value=85
test-errors.csv:3 type=ds18b20 sensor_id=sensor005 value=85
```

## Project Structure

```
sensors-tools/
├── include/              # Header files
│   ├── csv_parser.h     # CSV file parsing
│   ├── json_parser.h    # JSON parsing
│   ├── error_detector.h # Error detection logic
│   └── file_utils.h     # File system utilities
├── src/                 # Implementation files
│   ├── csv_parser.cpp
│   ├── json_parser.cpp
│   ├── error_detector.cpp
│   ├── file_utils.cpp
│   ├── sensor-data.cpp  # Main application
│   └── main.cpp         # Entry point (refactored)
├── tests/               # Test suite
│   ├── test_csv_parser.cpp
│   ├── test_json_parser.cpp
│   ├── test_error_detector.cpp
│   ├── test_file_utils.cpp
│   └── run_tests.sh     # Linux test runner
├── .github/
│   └── workflows/
│       └── ci.yml       # GitHub Actions CI/CD
├── Makefile
└── README.md
```

## Development

### Adding New Features

The modular architecture makes it easy to extend:

1. **New parsers**: Add to `include/` and `src/`, implement parsing logic
2. **New error detectors**: Extend `ErrorDetector` class in `error_detector.cpp`
3. **New commands**: Add to `main.cpp` following the existing pattern

### Running Tests Locally

Tests can be run individually:
```bash
g++ -std=c++11 -Iinclude tests/test_csv_parser.cpp src/csv_parser.cpp -o test_csv_parser
./test_csv_parser
```

Or run the full suite:
```bash
make test
```


### Adding New Sensor Types

1. Inherit from the `Sensor` base class
2. Implement the `readValue()` method
3. Add your sensor to the `SensorManager`

Example:

```cpp
class HumiditySensor : public Sensor {
public:
    HumiditySensor(const std::string& name)
        : Sensor(name, "humidity") {}
    
    double readValue() override {
        // Your implementation here
        return 0.0;
    }
};
```

## License

GPL-3.0 or later. See [debian/copyright](debian/copyright) for details.

## Author

Your Name <your.email@example.com>
