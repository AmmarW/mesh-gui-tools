# MeshX

MeshX is a powerful and versatile tool designed for handling and processing 3D surface and volume meshes. It provides a comprehensive set of features, including mesh import/export, validation, transformations, and advanced boolean operations. With a user-friendly graphical interface and robust documentation, MeshX is ideal for researchers, engineers, and developers working on computational geometry, CAD, and 3D modeling projects.
### Build Status

| Feature                                    | Status     |
|--------------------------------------------|------------|
| Surface Meshes Import/Export Handling      | ![Completed](https://img.shields.io/badge/status-completed-brightgreen)  |
| Mesh Validation                            | ![Completed](https://img.shields.io/badge/status-completed-brightgreen)  |
| Transformation (Translation/Rotation/Scaling) | ![Completed](https://img.shields.io/badge/status-completed-brightgreen)  |
| Boolean Operations (Union/Intersection/Difference) | ![Completed](https://img.shields.io/badge/status-completed-brightgreen)  |
| Volume Mesh Generation                     | ![Completed](https://img.shields.io/badge/status-completed-brightgreen)  |
| Graphical User Interface (GUI)             | ![Completed](https://img.shields.io/badge/status-completed-brightgreen)  |
| Documentation                              | ![Completed](https://img.shields.io/badge/status-completed-brightgreen)  |

### Clone Instructions

Make sure you clone with `--recurse-submodules` enabled to get the required libraries:

```sh
git clone --recurse-submodules link/to/repo
```

### Build Instructions

#### For Windows (using VS 2022):

```sh
# Navigate to the project directory
cd CodeForces_tools

# Generate the build files
cmake -S . -B build -G "Visual Studio 17 2022"

# Build the project
cmake --build build --config Release
```

The resulting executable `MeshX.exe` can be found in the `build\Release` directory.
### CGAL Setup

```sh
# Clone the vcpkg repository
git clone https://github.com/microsoft/vcpkg.git

# Navigate to the vcpkg directory
cd .\vcpkg\

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat -disableMetrics

# Set environment variables
$env:VCPKG_ROOT = "C:\vcpkg"
$env:PATH = "$env:VCPKG_ROOT;$env:PATH"

# Install CGAL and TBB
vcpkg install cgal
vcpkg install tbb
```

### Confirm Location and Add System and Environment Paths

Ensure the following paths are added to your system and environment variables:

- `C:\vcpkg`
- `C:\vcpkg\packages\gmp_x64-windows\bin`
- `C:\vcpkg\packages\mpfr_x64-windows\bin`
- `C:\vcpkg\installed\x64-windows\bin`

### CGAL Build

1. Configure the project with vcpkg:
    ```sh
     # build using VS (version)
     cmake -S . -B build -G "Visual Studio 17 2022"
     # specify vcpkg if build fails
     cmake -B build -S . -DCMAKE_PREFIX_PATH=C:/path/to/vcpkg/installed/x64-windows
     ```
2. Build the project:
    ```sh
    cmake --build build --config Release
    ```