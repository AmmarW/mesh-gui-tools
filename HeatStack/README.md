# HeatStack

HeatStack is a computational framework designed for solving heat transfer problems efficiently. It provides tools for simulation, analysis, and visualization of heat distribution in various systems.

## Features

- **Modular Design**: Easily extendable and customizable.
- **High Performance**: Optimized for large-scale simulations.
- **Cross-Platform**: Compatible with Windows, Linux, and macOS.
- **Visualization Tools**: Built-in support for visualizing heat distribution.

## Project File Structure

HeatStack/

├── **`include/`**: Header files for the project.

│   ├── BTCSMatrixSolver.h

│   ├── BoundaryConditions.h

│   ├── CLI.h

│   ├── HeatEquationSolver.h

│   ├── InitialTemperature.h

│   ├── MaterialProperties.h

│   ├── MeshHandler.h

│   ├── SafetyArbitrator.h

│   ├── TemperatureComparator.h

│   ├── TemperatureDistribution.h

│   ├── TimeHandler.h

├── **`src/`**: Source code implementation.

│   ├── BTCSMatrixSolver.cpp

│   ├── BoundaryConditions.cpp

│   ├── CLI.cpp

│   ├── HeatEquationSolver.cpp

│   ├── InitialTemperature.cpp

│   ├── MaterialProperties.cpp

│   ├── MeshHandler.cpp

│   ├── SafetyArbitrator.cpp

│   ├── TemperatureComparator.cpp

│   ├── TemperatureDistribution.cpp

│   ├── TimeHandler.cpp

│   ├── main.cpp

│   ├── initial_temperature.csv

├── **`tests/`**: Unit tests for the project.

│   ├── test_boundary_conditions.cpp

│   ├── test_humanoid_mesh.cpp

│   ├── test_main.cpp

│   ├── test_material_properties.cpp

│   ├── test_heat_equation_solver.cpp

│   ├── CMakeLists.txt

├── .gitignore

├── README.md

├── **`build/`**: Build files and compiled binaries.


## Project Workflow

### Inputs
- Mesh file (.obj): Robot surface mesh in .obj format, processed by MeshHandler.



- Stack configuration (via MaterialProperties): Each stack (TPS, carbon-fiber, glue, steel) has varying thicknesses based on l/L (head to toe, L=2.5m).



- Boundary conditions: Dirichlet (exhaust gas temperature at t=0, then 300K water) on TPS surface, Neumann (zero flux) on steel surface.



- Initial Conditions: 

    - Initial temperature is 300K (room temperature) for stacks

    - Initial external temperature profile head to toe (via MaterialProperties)


- Simulation Parameters: 

    - Simulation durations: 1, 3, 7 hours.

    - Grid Spacing: Non-uniform, constant

    - Time Step Size: Adaptive (while satisfying CFL stability and error criteria)


### Goals: 
- Solve 1D-Heat Equation 
    - Via BTCS or Crank Nicholson Schemes 

    - Tridiagonal matrix inversion 

- Minimize TPS thickness to keep steel temperature < 800K.

- Optimize number of stacks from head to toe (test 100, 500, 1000).

### Outputs

- Temperature distributions across stack depth

- TPS thickness profile

## Getting Started

### Prerequisites

- C++ compiler (e.g., GCC, Clang, or MSVC)
- CMake (version 3.10 or higher)

### Building the Project

1. Clone the repository and build in root of HeatStack:
    ```sh
     # build using VS (version)
     cmake -S . -B build -G "Visual Studio 17 2022"



- Work divide:

Ammar - BTCSMatrixSolver.cpp, HeatEquationSolver.cpp
Jasdeep - MeshHandler.cpp, BoundaryCondition.cpp
Harsh - main.cpp, CLI.cpp, TimeHandler.cpp
Shivam - InitialTemperature.cpp, TemperatureDistribution.cpp
Mohini - TemperatureComparator.cpp, HeatEquationSolver.cpp
