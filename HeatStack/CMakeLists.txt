cmake_minimum_required(VERSION 3.10)
project(HeatStackGUI)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Vcpkg (optional, only if you are using it)
set(CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows")

# Find packages
find_package(OpenGL REQUIRED)
#find_package(glfw3 CONFIG REQUIRED)

# Include directories
include_directories(
    include
    ../third-party/imgui
    ../third-party/imgui/backends
    ../third-party/tinyfiledialogs
    # C:/vcpkg/installed/x64-windows/include  # ❓ COMMENT THIS unless CMake fails to find something automatically
)

# --- Add subdirectory for third-party GLFW only if you are manually building glfw ---
add_subdirectory(../third-party/glfw ../build/third-party/glfw)  # ❓ COMMENT THIS if using vcpkg glfw

# ImGui sources
set(IMGUI_SRC
    ../third-party/imgui/imgui.cpp
    ../third-party/imgui/imgui_draw.cpp
    ../third-party/imgui/imgui_tables.cpp
    ../third-party/imgui/imgui_widgets.cpp
    ../third-party/imgui/backends/imgui_impl_glfw.cpp
    ../third-party/imgui/backends/imgui_impl_opengl3.cpp
)

# --- Executable ---
add_executable(HeatStackGUI
    # src/main.cpp   # ❌ COMMENT OUT: Old CLI main. Causes conflict with GUI
    src/main_gui.cpp  # ✅ New main for GUI
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
    ../third-party/tinyfiledialogs/tinyfiledialogs.c   # ✅ Added for file dialogs
)

# --- Link Libraries ---
target_link_libraries(HeatStackGUI
    glfw
    ${OPENGL_gl_LIBRARY}
)

# --- Optional: Custom command to copy icon if needed ---
# (You had this in MeshX. Only if you use an icon like icon.png)
# add_custom_command(TARGET HeatStackGUI POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different
#     "${CMAKE_SOURCE_DIR}/media/icon.png"
#     $<TARGET_FILE_DIR:HeatStackGUI>
# )
