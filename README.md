# Sensor Tools

A C++ library and command-line tool for managing and reading sensor data.

## Features

- Sensor management framework
- Support for multiple sensor types (temperature, etc.)
- Shared library for integration into other applications
- Command-line interface for sensor interaction

## Building from Source

### Prerequisites

```bash
sudo apt-get install build-essential cmake debhelper
```

### Build

```bash
mkdir build
cd build
cmake ..
make
```

### Install

```bash
sudo make install
```

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

Convert JSON sensor data files to CSV:

```bash
# Single file
sensor-data convert input.out output.csv

# Multiple files
sensor-data convert sensor1.out sensor2.out sensor3.out output.csv

# Directory of files (non-recursive by default)
sensor-data convert /path/to/sensor/logs/ output.csv

# Directory with extension filter
sensor-data convert -e .out /path/to/sensor/logs/ output.csv

# Recursive directory traversal
sensor-data convert -r /path/to/sensor/logs/ output.csv

# Recursive with extension filter
sensor-data convert -r -e .out /path/to/sensor/logs/ output.csv

# Mixed input
sensor-data convert file1.out file2.out /path/to/dir/ output.csv
```

**Options:**
- `-r, --recursive` - Recursively process subdirectories
- `-e, --extension <ext>` - Filter files by extension (e.g., `.out` or `out`)

The `sensor-data` tool reads JSON-formatted sensor data files (one JSON object per line)
and converts them to CSV format. Each sensor reading becomes a row in the output CSV.

## Development

### Project Structure

```
sensor-tools/
├── CMakeLists.txt          # CMake build configuration
├── README.md               # This file
├── include/
│   └── sensor.h            # Public header files
├── src/
│   ├── main.cpp            # Command-line application
│   └── sensor.cpp          # Library implementation
└── debian/                 # Debian packaging files
    ├── changelog           # Package changelog
    ├── compat              # Debhelper compatibility level
    ├── control             # Package metadata
    ├── copyright           # Copyright and license info
    ├── rules               # Build rules
    ├── source/
    │   └── format          # Source package format
    └── *.install           # Installation file lists
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
