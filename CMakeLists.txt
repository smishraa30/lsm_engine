cmake_minimum_required(VERSION 3.10)
project(LSMEngine VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

add_executable(lsm_test src/main.cpp src/memtable.cpp)
add_executable(lsm_test src/main.cpp src/memtable.cpp src/bloom_filter.cpp)
