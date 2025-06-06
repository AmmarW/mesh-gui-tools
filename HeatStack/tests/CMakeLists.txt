# Define source files for the main HeatStack library
set(HEATSTACK_SOURCES
    ../src/BTCSMatrixSolver.cpp
    ../src/BoundaryConditions.cpp
    ../src/CLI.cpp
    ../src/HeatEquationSolver.cpp
    ../src/InitialTemperature.cpp
    ../src/MaterialProperties.cpp
    ../src/MeshHandler.cpp
    ../src/SafetyArbitrator.cpp
    ../src/TemperatureComparator.cpp
    ../src/TemperatureDistribution.cpp
    ../src/TimeHandler.cpp
)

# Test for HeatEquationSolver
add_executable(TestHeatEquationSolver test_heat_equation_solver.cpp ${HEATSTACK_SOURCES})
target_include_directories(TestHeatEquationSolver PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME TestHeatEquationSolver COMMAND TestHeatEquationSolver)

# Test for MaterialProperties
add_executable(TestMaterialProperties test_material_properties.cpp ${HEATSTACK_SOURCES})
target_include_directories(TestMaterialProperties PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME TestMaterialProperties COMMAND TestMaterialProperties)

# Other tests (assuming similar structure)
add_executable(TestBoundaryConditions test_boundary_conditions.cpp ${HEATSTACK_SOURCES})
target_include_directories(TestBoundaryConditions PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME TestBoundaryConditions COMMAND TestBoundaryConditions)

add_executable(TestHumanoidMesh test_humanoid_mesh.cpp ${HEATSTACK_SOURCES})
target_include_directories(TestHumanoidMesh PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME TestHumanoidMesh COMMAND TestHumanoidMesh)

add_executable(TestMain test_main.cpp ${HEATSTACK_SOURCES})
target_include_directories(TestMain PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME TestMain COMMAND TestMain)

# Enable testing
enable_testing()