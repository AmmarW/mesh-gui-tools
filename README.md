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