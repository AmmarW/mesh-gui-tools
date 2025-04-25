#ifndef MATERIAL_PROPERTIES_H
#define MATERIAL_PROPERTIES_H

#include <vector>
#include <string>
#include <array>

// Structure to hold material properties
struct Material {
    std::string name;           // Material name (e.g., TPS, carbon-fiber)
    double k;                   // Thermal conductivity (W/m*K)
    double rho;                 // Density (kg/m^3)
    double c;                   // Specific heat capacity (J/kg*K)
    double maxTemp;             // Max allowable temperature (K), e.g., 800K for steel
    double glassTransitionTemp; // Glass transition temperature (K), 0 if not applicable
};

// Structure to define a layer in the stack
struct Layer {
    Material material;          // Material properties
    double thickness;           // Thickness (m)
    int numPoints;              // Number of grid points in this layer
};

// Structure to define a stack (TPS, carbon-fiber, glue, steel)
struct Stack {
    int id;                     // Stack ID
    std::vector<Layer> layers;  // Layers in the stack (outer to inner)
    double totalThickness;      // Total thickness (m)
    std::vector<double> xGrid;  // Grid points across the stack
};

// Class to manage material properties and stack configurations
class MaterialProperties {
public:
    MaterialProperties();
    ~MaterialProperties();

    // Load stack configurations from a file or predefined data
    void loadStacks(const std::string& filename);

    // Get stack by ID
    Stack getStack(int id) const;

    // Generate grid points for a stack, ensuring interface alignment
    void generateGrid(Stack& stack, int pointsPerLayer = 10);

    // Map thickness profiles based on non-dimensional position (l/L)
    double getTPSThickness(double l_over_L) const;
    double getCarbonFiberThickness(double l_over_L) const;
    double getGlueThickness(double l_over_L) const;
    double getSteelThickness(double l_over_L) const;

    // Get TPS thickness bounds
    double getMinTPSThickness() const { return 0.0001; } // 0.01 cm
    double getMaxTPSThickness() const { return 0.01; }   // 1 cm

private:
    std::vector<Stack> stacks;  // List of stacks
};

#endif // MATERIAL_PROPERTIES_H