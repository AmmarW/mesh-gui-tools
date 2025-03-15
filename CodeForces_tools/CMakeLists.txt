cmake_minimum_required(VERSION 3.10)
project(CodeForces_Mesh_Handler)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add include directories
include_directories(include)
include_directories(third-party/imgui)
include_directories(third-party/imgui/backends)
include_directories(third-party/tinyfiledialogs)

# Add subdirectories for third-party libraries
add_subdirectory(third-party/glfw)

# Add source files for third-party libraries
set(IMGUI_SRC
    third-party/imgui/imgui.cpp
    third-party/imgui/imgui_draw.cpp
    third-party/imgui/imgui_tables.cpp
    third-party/imgui/imgui_widgets.cpp
    third-party/imgui/backends/imgui_impl_glfw.cpp
    third-party/imgui/backends/imgui_impl_opengl3.cpp
)


# Add the executable
add_executable(CodeForces_Mesh_Handler
    src/main.cpp
    src/MeshdataExporter.cpp
    src/MeshMetadata.cpp
    src/MeshValidator.cpp
    src/ObjExporter.cpp
    src/ObjParser.cpp
    src/MeshTransform.cpp
    third-party/tinyfiledialogs/tinyfiledialogs.c
    ${IMGUI_SRC}
)

# Link libraries
find_package(OpenGL REQUIRED)
target_link_libraries(CodeForces_Mesh_Handler
    glfw
    ${OPENGL_gl_LIBRARY}
)