cmake_minimum_required(VERSION 3.10)
project(HeatStack)

set(CMAKE_CXX_STANDARD 17)

# Include headers
include_directories(${PROJECT_SOURCE_DIR}/include)

# Collect all source files
file(GLOB SRC_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(HeatStack ${SRC_FILES})

# Enable testing and add tests directory
enable_testing()
add_subdirectory(tests)
