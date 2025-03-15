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




## CGAL Setup:
git clone https://github.com/microsoft/vcpkg.git
cd .\vcpkg\
.\bootstrap-vcpkg.bat -disableMetrics
$env:VCPKG_ROOT = "C:\vcpkg"
$env:PATH = "$env:VCPKG_ROOT;$env:PATH"

vcpkg install cgal
vcpkg install tbb

## CGAL Build
cmake -B build -S . -DCMAKE_PREFIX_PATH=C:/vcpkg/installed/x64-windows
cmake --build build --config Release