# Sensor Tools

[![CI](https://github.com/Wildlife-Systems/sensor-tools/actions/workflows/ci.yml/badge.svg)](https://github.com/Wildlife-Systems/sensor-tools/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/Wildlife-Systems/sensor-tools/graph/badge.svg)](https://codecov.io/gh/Wildlife-Systems/sensor-tools)

A command-line tool for processing and analysing sensor data files.

## Features

- Transform between JSON and CSV formats
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

### transform

Transform sensor data files between formats.

```bash
# JSON to CSV
sensor-data transform -F csv input.out output.csv

# JSON to JSON (streaming, preserves format)
sensor-data transform input.out output.out

# Filter by date range
sensor-data transform --min-date 2026-01-01 --max-date 2026-01-31 input.out
```

### list-rejects

List rejected readings (inverse of transform filters). Useful for inspecting which readings would be filtered out.

```bash
# Show error readings that would be removed
sensor-data list-rejects --remove-errors input.out

# Show readings that would be filtered by --clean
sensor-data list-rejects --clean input.out

# Show rows with empty values
cat data.out | sensor-data list-rejects --not-empty value

# Show readings outside date range
sensor-data list-rejects --min-date 2026-01-01 input.out

# Remove error readings
sensor-data transform --remove-errors input.out output.out

# Stdin to stdout (streaming)
cat sensors.out | sensor-data transform

# Recursive directory processing
sensor-data transform -r -e .out /path/to/logs/ output.csv
```

**Options:**
- `-of, --output-format <format>` - Output format: `json` (default) or `csv`
- `-o <file>` - Output file (default: stdout)
- `-if, --input-format <format>` - Input format: `json` or `csv` (auto-detected)
- `-r, --recursive` - Recursively process subdirectories
- `-e, --extension <ext>` - Filter files by extension (e.g., `.out`)
- `-d, --depth <n>` - Maximum recursion depth
- `--min-date <date>` - Include only readings on or after this date
- `--max-date <date>` - Include only readings on or before this date
- `--remove-errors` - Remove error readings (DS18B20 value=85 or -127)
- `--remove-whitespace` - Remove extra whitespace from output (compact format)
- `--remove-empty-json` - Remove empty JSON input lines (e.g., `[{}]`, `[]`)
- `--not-empty <column>` - Skip rows where column is empty
- `--only-value <col:val>` - Only include rows where column equals value
- `--allowed-values <column> <values|file>` - Only include rows where column is in allowed values
- `--clean` - Shorthand for `--remove-empty-json --not-empty value --remove-errors`
- `--tail <n>` - Only read the last n lines from each file
- `-v` - Verbose output
- `-V` - Very verbose output

### count

Count sensor data readings with optional filters.

```bash
# Count all readings in a file
sensor-data count input.out

# Count from stdin
cat sensors.out | sensor-data count

# Count with filters
sensor-data count --remove-errors input.out
sensor-data count --only-value type:temperature input.out

# Count recursively
sensor-data count -r -e .out /path/to/logs/
```

**Options:**
- `-if, --input-format <format>` - Input format: `json` or `csv` (auto-detected)
- `-f, --follow` - Follow mode: continuously read stdin and update count
- `-r, --recursive` - Recursively process subdirectories
- `-e, --extension <ext>` - Filter files by extension (e.g., `.out`)
- `-d, --depth <n>` - Maximum recursion depth
- `--min-date <date>` - Include only readings on or after this date
- `--max-date <date>` - Include only readings on or before this date
- `--remove-errors` - Remove error readings (DS18B20 value=85 or -127)
- `--remove-empty-json` - Remove empty JSON input lines
- `--not-empty <column>` - Skip rows where column is empty
- `--only-value <col:val>` - Only include rows where column equals value
- `--exclude-value <col:val>` - Exclude rows where column equals value
- `--allowed-values <column> <values|file>` - Only include rows where column is in allowed values
- `--clean` - Shorthand for `--remove-empty-json --not-empty value --remove-errors`
- `--tail <n>` - Only read the last n lines from each file
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

# Clean statistics (exclude empty values and errors)
sensor-data stats --clean input.out
```

**Options:**
- `-c, --column <name>` - Analyze only this column (default: value, use 'all' for all columns)
- `-if, --input-format <format>` - Input format: `json` or `csv` (auto-detected)
- `-f, --follow` - Follow mode: continuously read input and update stats
- `--only-value <col:val>` - Only include rows where column equals value
- `--exclude-value <col:val>` - Exclude rows where column equals value
- `--allowed-values <column> <values|file>` - Only include rows where column is in allowed values
- `--not-empty <column>` - Skip rows where column is empty
- `--remove-empty-json` - Remove empty JSON input lines
- `--remove-errors` - Remove error readings (DS18B20 value=85 or -127)
- `--clean` - Shorthand for `--remove-empty-json --not-empty value --remove-errors`
- `--tail <n>` - Only read the last n lines from each file
- `-r, --recursive` - Recursively process subdirectories
- `-v` - Verbose output
- `-V` - Very verbose output

**Statistics Output:**
- **Basic stats:** Count, Min, Max, Range, Mean, StdDev
- **Quartiles:** Q1 (25%), Median, Q3 (75%), IQR
- **Outliers:** Count and percent using 1.5×IQR method
- **Delta stats:** Min, Max, Mean of consecutive changes, Volatility (std dev of deltas)
- **Max Jump:** Largest single value change with before/after values

**Time-based stats** (when timestamp field is present):
- **Time Range:** First and last timestamps with human-readable dates
- **Duration:** Total time span in days/hours/minutes/seconds
- **Rate:** Readings per hour and per day
- **Sampling:** Typical interval between readings
- **Gap Detection:** Number of gaps (>3× typical interval) and max gap size

Example output:
```
Statistics:

Time Range:
  First:     2023-11-14 22:13:20 (1700000000)
  Last:      2023-11-15 01:13:20 (1700010800)
  Duration:  3h 0m 0s (10800 seconds)
  Rate:      1.33 readings/hour, 32.00 readings/day

  Sampling:
    Typical interval: 3600s
    Gaps detected:    0

value:
  Count:    100
  Min:      18.5
  Max:      26.3
  Range:    7.8
  Mean:     22.1
  StdDev:   1.8

  Quartiles:
    Q1 (25%):  20.5
    Median:    22.0
    Q3 (75%):  23.5
    IQR:       3.0

  Outliers (1.5*IQR):
    Count:     2
    Percent:   2.0%

  Delta (consecutive changes):
    Min:       0.1000
    Max:       2.5000
    Mean:      0.5000
    Volatility:0.3000

  Max Jump:
    Size:      2.5000
    From:      20.0 -> 22.5
```

### latest

Find the latest timestamp for each sensor.

```bash
# Get latest timestamp per sensor
sensor-data latest input.out

# Limit to first n sensors (alphabetically by sensor_id)
sensor-data latest -n 5 input.out

# Limit to last n sensors
sensor-data latest -n -5 input.out

# With date filtering
sensor-data latest --min-date 2026-01-01 input.out
```

**Options:**
- `-n <num>` - Limit output rows (positive = first n, negative = last n)
- `-of, --output-format <fmt>` - Output format: `human` (default), `csv`, or `json`
- `-if, --input-format <format>` - Input format: `json` or `csv` (auto-detected)
- `--min-date <date>` - Include only readings on or after this date
- `--max-date <date>` - Include only readings on or before this date
- `--tail <n>` - Only read the last n lines from each file
- `-r, --recursive` - Recursively process subdirectories
- `-e, --extension <ext>` - Filter files by extension (e.g., `.out`)
- `-v` - Verbose output

**Output columns:** `sensor_id`, `unix_timestamp`, `iso_date`

Example output:
```
sensor001,1737315700,2025-01-19 19:41:40
sensor002,1737315650,2025-01-19 19:40:50
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
│   ├── data_counter.h        # count command
│   ├── data_reader.h         # Unified data reading
│   ├── date_utils.h          # Date parsing utilities
│   ├── error_detector.h      # Error detection
│   ├── error_lister.h        # list-errors command
│   ├── error_summarizer.h    # summarise-errors command
│   ├── file_collector.h      # File collection
│   ├── file_utils.h          # File utilities
│   ├── json_parser.h         # JSON parsing
│   ├── sensor_data_transformer.h # transform command
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
