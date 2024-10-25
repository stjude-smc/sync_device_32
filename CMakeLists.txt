cmake_minimum_required(VERSION 3.10)
project(unit_tests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)

# Include CppUTest
include_directories(${CMAKE_SOURCE_DIR}/cpputest/include)
add_subdirectory(${CMAKE_SOURCE_DIR}/cpputest)
include_directories(${CMAKE_SOURCE_DIR}/sync_device_32/src)
 
# Add your source files and tests
add_executable(unit_tests main.cpp)
target_link_libraries(unit_tests CppUTest)
