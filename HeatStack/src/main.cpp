#include "CLI.h"
#include "MeshHandler.h"
#include "MaterialProperties.h"
#include "InitialTemperature.h"
#include "HeatEquationSolver.h"
#include "TemperatureComparator.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>

int main(int argc, char* argv[]) {
    // Overall timer start
    auto overall_start = std::chrono::high_resolution_clock::now();

    CLI cli(argc, argv);
    if (cli.isHelpRequested()) return 0;

    // Load mesh and compute vertical bounds
    MeshHandler mesh;
    if (!mesh.loadMesh(cli.getMeshFile())) {
        std::cerr << "Error: cannot load mesh " << cli.getMeshFile() << "\n";
        return 1;
    }
    double zmin = mesh.getMinZ(), zmax = mesh.getMaxZ();
    double height = zmax - zmin;

    // Read initial temperature if provided
    InitialTemperature initTemp;
    std::vector<double> uniformInit;
    if (!cli.getInitFile().empty()) {
        uniformInit = initTemp.loadInitialTemperature(cli.getInitFile());
    }

    // Material properties
    MaterialProperties matProps;

    // Prepare output files
    std::ofstream summaryOut(cli.getOutputFile());
    summaryOut << "slice,l/L,method,finalSteelTemp,TPS_thickness\n";

    std::ofstream detailsOut("stack_details.csv");
    detailsOut << "slice,l/L,TPS_thickness,CarbonFiber_thickness,Glue_thickness,Steel_thickness,finalSteelTemp\n";

    // Simulation parameters from CLI
    int    nSlices       = cli.getNumSlices();
    int    pointsPerLayer= cli.getPointsPerLayer();
    double dt            = cli.getTimeStep();
    bool   adapt         = cli.useAdaptiveTimeStep();
    double tFinal        = cli.getTimeDuration();
    double theta         = cli.getTheta();

    double totalSolveMs = 0.0;

    // Loop over slices
    for (int slice = 0; slice < nSlices; ++slice) {
        double z = zmin + (double(slice)/(nSlices-1)) * height;
        double lL = (z - zmin) / height;

        // Build stack layers with full material definitions
        Stack s;
        s.id = slice + 1;
        s.layers = {
            {{"TPS",         0.2,  160.0, 1200.0,   0.0, 1200.0}, matProps.getTPSThickness(lL),          pointsPerLayer},
            {{"CarbonFiber", 500.0,1600.0, 700.0,   0.0,  350.0}, matProps.getCarbonFiberThickness(lL),     pointsPerLayer},
            {{"Glue",        200.0,1300.0, 900.0,   0.0,  400.0}, matProps.getGlueThickness(lL),            pointsPerLayer},
            {{"Steel",       100.0,7850.0, 500.0, 800.0,    0.0}, matProps.getSteelThickness(lL),           pointsPerLayer}
        };
        matProps.generateGrid(s, pointsPerLayer);

        // Initialize solver and time handler
        TimeHandler th(tFinal, dt, adapt);
        HeatEquationSolver solver(theta);
        solver.initialize(s, th);
        
        // Set initial temperature
        if (!uniformInit.empty()) {
            solver.setInitialTemperature(uniformInit);
        } else {
            solver.setInitialTemperature(std::vector<double>(s.xGrid.size(), 300.0));
        }

        // Boundary conditions
        solver.setBoundaryConditions(
            new DirichletCondition(static_cast<float>(matProps.getExhaustTemp(lL))),
            new NeumannCondition(0.0f)
        );

        // Timer for solver per slice
        auto solve_start = std::chrono::high_resolution_clock::now();

        // Time-marching loop
        while (!th.isFinished()) {
            solver.step();
            th.advance();
        }

        auto solve_end = std::chrono::high_resolution_clock::now();
        double solveMs = std::chrono::duration<double, std::milli>(solve_end - solve_start).count();
        totalSolveMs += solveMs;

        // Extract results
        const auto& Tdist = solver.getTemperatureDistribution();
        double steelT     = Tdist.back();
        double tpsThick   = matProps.getTPSThickness(lL);
        double cfThick    = matProps.getCarbonFiberThickness(lL);
        double glueThick  = matProps.getGlueThickness(lL);
        double steelThick = matProps.getSteelThickness(lL);

        // Suggest TPS thickness (optional optimization)
        TemperatureComparator comp;
        double tpsOpt = comp.suggestTPSThickness(s, 800.0, tFinal, lL, matProps);

        // Write summary CSV
        summaryOut << slice+1 << "," << lL << ",BTCS," << steelT << "," << tpsOpt << "\n";

        // Write details CSV
        detailsOut << slice+1 << ","
                   << lL    << ","
                   << tpsThick  << ","
                   << cfThick   << ","
                   << glueThick << ","
                   << steelThick<< ","
                   << steelT    << "\n";
    }

    summaryOut.close();
    detailsOut.close();

    // Overall timer end
    auto overall_end = std::chrono::high_resolution_clock::now();
    double overallMs = std::chrono::duration<double, std::milli>(overall_end - overall_start).count();

    // Print timing info
    std::cout << "Total solver time: " << totalSolveMs << " ms\n";
    std::cout << "Overall program time: " << overallMs << " ms\n";

    return 0;
}
