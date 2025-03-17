### Shared Repo for MEEN-689: Applied Computing Concepts

#### Group Members
- Ammar Waheed
- Harsh Mittal
- Jasdeep Bajaj
- Mohini Priya Kolluri
- Shivam Vashi

### Build Instructions for Branch 'GUI'

#### For Windows (using VS 2022):

1. Navigate to the project directory:
    ```sh
    cd CodeForces_tools
    ```
2. Generate the build files:
    ```sh
    cmake -S . -B build -G "Visual Studio 17 2022"
    ```
3. Build the project:
    ```sh
    cmake --build build --config Release
    ```

The resulting executable `CodeForces_Mesh_Handler.exe` can be found in the `build\Release` directory.
### CGAL Setup

1. Clone the vcpkg repository:
    ```sh
    git clone https://github.com/microsoft/vcpkg.git
    ```
2. Navigate to the vcpkg directory:
    ```sh
    cd .\vcpkg\
    ```
3. Bootstrap vcpkg:
    ```sh
    .\bootstrap-vcpkg.bat -disableMetrics
    ```
4. Set environment variables:
    ```sh
    $env:VCPKG_ROOT = "C:\vcpkg"
    $env:PATH = "$env:VCPKG_ROOT;$env:PATH"
    ```
5. Install CGAL and TBB:
    ```sh
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
     cmake -B build -S . -DCMAKE_PREFIX_PATH=C:/path-to/vcpkg/installed/x64-windows
     ```
2. Build the project:
    ```sh
    cmake --build build --config Release
    ```
