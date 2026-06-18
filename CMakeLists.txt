cmake_minimum_required(VERSION 3.10)
project(LSMEngine VERSION 1.0)

# Enforce C++17 Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Tell CMake where to find our blueprint headers
include_directories(include)

# Compile the engine and link the source files
add_executable(lsm_test src/main.cpp src/memtable.cpp)
add_executable(lsm_test src/main.cpp src/memtable.cpp src/bloom_filter.cpp)