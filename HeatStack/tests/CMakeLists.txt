cmake_minimum_required(VERSION 3.10)
project(HeatStackTests)

set(CMAKE_CXX_STANDARD 17)

include_directories(${PROJECT_SOURCE_DIR}/../include)

# Collect all test source files
file(GLOB TEST_SRC_FILES "${PROJECT_SOURCE_DIR}/*.cpp")

add_executable(HeatStackTests ${TEST_SRC_FILES})
add_test(NAME HeatStackTests COMMAND HeatStackTests)
