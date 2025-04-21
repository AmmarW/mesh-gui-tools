# HeatStack

HeatStack is a computational framework designed for solving heat transfer problems efficiently. It provides tools for simulation, analysis, and visualization of heat distribution in various systems.

## Features

- **Modular Design**: Easily extendable and customizable.
- **High Performance**: Optimized for large-scale simulations.
- **Cross-Platform**: Compatible with Windows, Linux, and macOS.
- **Visualization Tools**: Built-in support for visualizing heat distribution.

## Project Structure

- **`include/`**: Header files for the project.
- **`src/`**: Source code implementation.
- **`tests/`**: Unit tests for the project.
- **`build/`**: Build files and compiled binaries.

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
