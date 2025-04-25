#include "CLI.h"
#include "TimeHandler.h"
#include "InitialTemperature.h"
#include "TemperatureDistribution.h"
#include "MeshHandler.h"
#include "MaterialProperties.h"
#include "HeatEquationSolver.h"
#include "TemperatureComparator.h"
#include "SafetyArbitrator.h"
#include <iostream>
#include <fstream>
#include <vector>

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    CLI cli(argc, argv);
    if (cli.isHelpRequested()) {
        return 0;
    }

    // Initialize components
    MeshHandler meshHandler;
    MaterialProperties materialProps;
    InitialTemperature initialTempHandler;
    TemperatureDistribution tempDistHandler;
    TemperatureComparator tempComparator;
    SafetyArbitrator safetyArbitrator;

    // Load mesh
    if (!meshHandler.loadMesh(cli.getMeshFile())) {
        std::cerr << "Failed to load mesh" << std::endl;
        return 1;
    }

    // Simulation durations
    std::vector<double> durations = {3600, 3 * 3600, 7 * 3600}; // 1, 3, 7 hours

    // Grid-independence and stack convergence study
    std::vector<int> pointsPerLayer = {5, 10, 20};
    std::vector<int> numStacks = {5, 10, 20}; // Test different numbers of stacks
    std::ofstream output("results.csv");
    output << "StackID,l/L,Points,Stacks,Duration,Method,TPS_Thickness,Steel_Temp\n";

    // Generate representative stacks
    for (int nStacks : numStacks) {
        std::vector<Stack> stacks;
        for (int i = 0; i < nStacks; ++i) {
            double l_over_L = static_cast<double>(i) / (nStacks - 1);
            Stack stack;
            stack.id = i + 1;
            stack.layers = {
                {{"TPS", 0.2, 160, 1200, 0, 1200}, materialProps.getTPSThickness(l_over_L), 10},
                {{"CarbonFiber", 500, 1600, 700, 0, 350}, materialProps.getCarbonFiberThickness(l_over_L), 10},
                {{"Glue", 200, 1300, 900, 0, 400}, materialProps.getGlueThickness(l_over_L), 10},
                {{"Steel", 100, 7850, 500, 800, 0}, materialProps.getSteelThickness(l_over_L), 10}
            };
            materialProps.generateGrid(stack);
            stacks.push_back(stack);
        }

        // Process each stack
        for (const auto& stack : stacks) {
            double l_over_L = (stack.id - 1.0) / (nStacks - 1.0);
            for (int points : pointsPerLayer) {
                Stack testStack = stack;
                materialProps.generateGrid(testStack, points);

                for (double duration : durations) {
                    // BTCS (θ=1)
                    std::vector<double> tempsBTCS = tempComparator.runSimulation(testStack, duration, 1.0, l_over_L);
                    double tpsThickness = tempComparator.suggestTPSThickness(testStack, 800.0, duration, l_over_L, materialProps);
                    bool safe = safetyArbitrator.evaluate(tempsBTCS, 800.0);
                    output << stack.id << "," << l_over_L << "," << points << "," << nStacks << "," << duration << ",BTCS," << tpsThickness << "," << tempsBTCS.back() << "\n";

                    // Crank-Nicolson (θ=0.5)
                    std::vector<double> tempsCN = tempComparator.runSimulation(testStack, duration, 0.5, l_over_L);
                    tpsThickness = tempComparator.suggestTPSThickness(testStack, 800.0, duration, l_over_L, materialProps);
                    safe = safetyArbitrator.evaluate(tempsCN, 800.0);
                    output << stack.id << "," << l_over_L << "," << points << "," << nStacks << "," << duration << ",CN," << tpsThickness << "," << tempsCN.back() << "\n";
                }
            }
        }
    }
    output.close();
    std::cout << "Results exported to results.csv" << std::endl;
    return 0;
}