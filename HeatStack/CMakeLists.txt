cmake_minimum_required(VERSION 3.10)
project(HeatStack)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows")

# Find packages
find_package(OpenGL REQUIRED)

# Add include directories
include_directories(include)
include_directories(../third-party/imgui)
include_directories(../third-party/imgui/backends)
include_directories(../third-party/tinyfiledialogs)
include_directories(C:/vcpkg/installed/x64-windows/include)

# Add subdirectories for third-party libraries
add_subdirectory(../third-party/glfw ../build/third-party/glfw)

# Add source files for third-party libraries
set(IMGUI_SRC
    ../third-party/imgui/imgui.cpp
    ../third-party/imgui/imgui_draw.cpp
    ../third-party/imgui/imgui_tables.cpp
    ../third-party/imgui/imgui_widgets.cpp
    ../third-party/imgui/backends/imgui_impl_glfw.cpp
    ../third-party/imgui/backends/imgui_impl_opengl3.cpp
)


# Collect all source files (bad practice imo - remove)
# file(GLOB SRC_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")
# add_executable(HeatStack ${SRC_FILES})

add_executable(HeatStack 
    src/main.cpp
    src/BoundaryConditions.cpp
    src/BTCSMatrixSolver.cpp
    src/CLI.cpp
    src/HeatEquationSolver.cpp
    src/InitialTemperature.cpp
    src/MaterialProperties.cpp
    src/MeshHandler.cpp
    src/SafetyArbitrator.cpp
    src/TemperatureComparator.cpp
    src/TemperatureDistribution.cpp
    src/TimeHandler.cpp
    ${IMGUI_SRC}
    # src/main_gui.cpp # uncomment to build this but make sure only one main func. call exists across main.cpp and main_gui.cpp
)

# Link libraries
target_link_libraries(HeatStack
    glfw
    ${OPENGL_gl_LIBRARY}
)


# Enable testing and add tests directory
# enable_testing()
# add_subdirectory(tests)
