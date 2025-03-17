cmake_minimum_required(VERSION 3.10)
project(CodeForces_Mesh_Handler)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows")

# Find packages
find_package(OpenGL REQUIRED)
find_package(CGAL CONFIG REQUIRED)
find_package(TBB REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Include directories for CGAL and Boost
include_directories(${CGAL_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
include(CGAL_TBB_support)

# Add include directories
include_directories(include)
include_directories(third-party/imgui)
include_directories(third-party/imgui/backends)
include_directories(third-party/tinyfiledialogs)
include_directories(C:/vcpkg/installed/x64-windows/include)

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

# Add the flag for concurrent mesh
add_definitions(-DCGAL_LINKED_WITH_TBB)
add_definitions(-DCGAL_CONCURRENT_MESH_3)


# Add the executable
add_executable(CodeForces_Mesh_Handler
    src/main.cpp
    src/MetadataExporter.cpp
    src/MeshMetadata.cpp
    src/MeshValidator.cpp
    src/ObjExporter.cpp
    src/ObjParser.cpp
    src/MeshTransform.cpp
    third-party/tinyfiledialogs/tinyfiledialogs.c
    ${IMGUI_SRC}
    src/AdaptiveMeshGenerator.cpp
    src/MeshConverter.cpp
    src/MeshBooleanOperations.cpp
)

# Link libraries
target_link_libraries(CodeForces_Mesh_Handler
    glfw
    ${OPENGL_gl_LIBRARY}
    CGAL::CGAL
    Boost::boost
    TBB::tbb
    TBB::tbbmalloc
    CGAL::TBB_support
    nlohmann_json::nlohmann_json
)