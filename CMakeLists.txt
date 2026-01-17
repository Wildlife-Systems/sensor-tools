cmake_minimum_required(VERSION 3.10)

# Set compiler to GCC
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

project(sensor-tools VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Executable
add_executable(sensor-data src/sensor-data.cpp)

# Installation rules
install(TARGETS sensor-data
    RUNTIME DESTINATION bin
)
