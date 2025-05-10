# HeatStack – 1D Composite‑Material Heat‑Equation Solver

HeatStack is a high‐performance 1D heat‐equation solver for composite materials under extreme, long‐duration thermal loads (inspired by NASA insulation design problems). You feed it any 3D .obj mesh, it slices it into 1D “stacks” along z, solves the transient heat equation in each stack (using BTCS or Crank–Nicholson), then runs a thermal comparator to recommend insulation thicknesses to meet your temperature criteria.

---

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

The resulting GUI executable `HeatStack.exe` can be found in the `build\Release` directory.

---

## Inputs

- **Mesh File**: 
  - Any 3D mesh (automatically sliced along the Z-axis into 1D stacks).
- **Simulation Parameters**: 
  - Specified through the GUI: stacks, simulation time, timestep, θ, boundary conditions, and inner-surface temperature goals.
- **Material Stacks**: 
  - Defined in `MaterialProperties.cpp` (Could be extended to GUI)

---

## Default Materials (Defined in `MaterialProperties.cpp`)

- **Insulation Material**: Requires thermal conductivity (W/m·K), density (kg/m³), and specific heat capacity (J/kg·K). Example: A low-conductivity material suitable for thermal protection systems (TPS).
- **Composite Material Layers**: Each layer requires thermal conductivity (W/m·K), density (kg/m³), and specific heat capacity (J/kg·K). Examples include high-conductivity materials like carbon fiber, adhesives like glue, or structural metals like steel.
