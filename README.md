# Sensor Tools

[![CI](https://github.com/Wildlife-Systems/sensor-tools/actions/workflows/ci.yml/badge.svg)](https://github.com/Wildlife-Systems/sensor-tools/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/Wildlife-Systems/sensor-tools/graph/badge.svg)](https://codecov.io/gh/Wildlife-Systems/sensor-tools)

A command-line tool for processing and analysing sensor data files.

## Features

- Convert between JSON and CSV formats
- Stream processing for large files (JSON to JSON)
- Filter by date range (`--min-date`, `--max-date`)
- Detect and filter sensor error readings (DS18B20 temperature errors)
- Calculate statistics (min, max, mean, median, stddev)
- Summarise error occurrences
- Recursive directory processing with depth limits
- Bash autocompletion

## Installation

### From Source

```bash
make
sudo make install
```

### Debian Package

```bash
dpkg-buildpackage -us -uc -b
sudo dpkg -i ../sensor-tools_*.deb
```

## Commands

### convert

Convert sensor data files between formats.

```bash
# JSON to CSV
sensor-data convert -F csv input.out output.csv

# JSON to JSON (streaming, preserves format)
sensor-data convert input.out output.out

# Filter by date range
sensor-data convert --min-date 2026-01-01 --max-date 2026-01-31 input.out

# Remove error readings
sensor-data convert --remove-errors input.out output.out

# Stdin to stdout (streaming)
cat sensors.out | sensor-data convert

# Recursive directory processing
sensor-data convert -r -e .out /path/to/logs/ output.csv
```

**Options:**
- `-F, --output-format <format>` - Output format: `json` (default) or `csv`
- `-o <file>` - Output file (default: stdout)
- `-f, --format <format>` - Input format: `json` or `csv` (auto-detected)
- `-r, --recursive` - Recursively process subdirectories
- `-e, --extension <ext>` - Filter files by extension (e.g., `.out`)
- `-d, --depth <n>` - Maximum recursion depth
- `--min-date <date>` - Include only readings on or after this date
- `--max-date <date>` - Include only readings on or before this date
- `--remove-errors` - Remove error readings (DS18B20 value=85 or -127)
- `--remove-whitespace` - Remove extra whitespace from output (compact format)
- `--not-empty <column>` - Skip rows where column is empty
- `--only-value <col:val>` - Only include rows where column equals value
- `-v` - Verbose output
- `-V` - Very verbose output

### list-errors

Find error readings in sensor data.

```bash
sensor-data list-errors -r -e .out /path/to/logs/
```

Output:
```
test-errors.out:2 type=ds18b20 sensor_id=sensor002 value=85
```

### summarise-errors

Summarise error occurrences by type.

```bash
sensor-data summarise-errors -r -e .out /path/to/logs/
```

Output:
```
DS18B20 communication error (value=85): 3 occurrences
DS18B20 disconnected or power-on reset (value=-127): 2 occurrences
Total errors: 5
```

### stats

Calculate statistics for numeric columns.

```bash
# Default: statistics for 'value' column only
sensor-data stats input.out

# Specific column
sensor-data stats -c temperature input.out

# All numeric columns
sensor-data stats -c all input.out

# From stdin
cat sensors.out | sensor-data stats
```

Output:
```
value:
  Count: 100
  Min: 18.5
  Max: 26.3
  Mean: 22.1
  Median: 22.0
  Std Dev: 1.8
```

## Date Formats

The `--min-date` and `--max-date` options accept:
- ISO 8601: `2026-01-17` or `2026-01-17T14:30:00`
- UK format: `17/01/2026`
- Unix timestamp: `1700000000`

## Input/Output Formats

### JSON (.out files)

Each line contains a JSON array with one or more sensor readings:
```json
[ { "sensor": "ds18b20", "value": 22.5, "unit": "Celsius" } ]
```

### CSV

Standard CSV with header row:
```csv
sensor,value,unit
ds18b20,22.5,Celsius
```

## Project Structure

```
sensors-tools/
├── include/                  # Header files
│   ├── common_arg_parser.h   # Shared argument parsing
│   ├── csv_parser.h          # CSV parsing
│   ├── data_reader.h         # Unified data reading
│   ├── date_utils.h          # Date parsing utilities
│   ├── error_detector.h      # Error detection
│   ├── error_lister.h        # list-errors command
│   ├── error_summarizer.h    # summarise-errors command
│   ├── file_collector.h      # File collection
│   ├── file_utils.h          # File utilities
│   ├── json_parser.h         # JSON parsing
│   ├── sensor_data_converter.h # convert command
│   └── stats_analyser.h      # stats command
├── src/
│   ├── sensor-data.cpp       # Main entry point
│   ├── csv_parser.cpp
│   ├── json_parser.cpp
│   ├── error_detector.cpp
│   └── file_utils.cpp
├── tests/                    # Test suite
├── completions/              # Shell completions
│   └── sensor-data.bash
├── debian/                   # Debian packaging
├── .github/workflows/        # CI/CD
└── Makefile
```

## Running Tests

```bash
# Unit tests
make test

# Integration tests (requires bash)
make test-integration

# All tests
make test-all
```

## License

GPL-3.0 or later. See [debian/copyright](debian/copyright) for details.
