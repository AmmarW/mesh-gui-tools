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
#include <algorithm> 
#include <sstream>
#include <fstream>

// convenience alias
using Clock = std::chrono::high_resolution_clock;
using MS    = std::chrono::duration<double, std::milli>;


int main(int argc, char* argv[]) {

    // === TIMERS ===
    double tMeshLoad           = 0.0;
    double tInitTempLoad       = 0.0;
    double tStackSetup         = 0.0;
    double tOrigSolve          = 0.0;
    double tHistOrigSave       = 0.0;
    double tOptSolve           = 0.0;
    double tOptSuggestion      = 0.0;
    double tHistOptSave        = 0.0;
    double tSummaryDetailsWrite= 0.0;

    // overall timer
    auto overall_start = Clock::now();;

    CLI cli(argc, argv);
    if (cli.isHelpRequested()) return 0;

    // ---- Mesh loading ----
    auto start = Clock::now();
    MeshHandler mesh;
    if (!mesh.loadMesh(cli.getMeshFile())) {
        std::cerr << "Error: cannot load mesh " 
                    << cli.getMeshFile() << "\n";
        return 1;
    }
    tMeshLoad = MS(Clock::now() - start).count();
    // grab bounds
    double zmin = mesh.getMinZ(), zmax = mesh.getMaxZ();
    double height = zmax - zmin;

    // ---- Initial temperature loading ----
    auto init_start = Clock::now();
    InitialTemperature initTemp;
    std::vector<double> uniformInit;
    if (!cli.getInitFile().empty()) {
        uniformInit = initTemp.loadInitialTemperature(cli.getInitFile());
    }
    tInitTempLoad = MS(Clock::now() - init_start).count();

    // Material properties
    MaterialProperties matProps;

    // Prepare summary + details CSVs
    std::ofstream summaryOut(cli.getOutputFile());
    summaryOut << "slice,l/L,method,finalSteelTemp,"
                << "TPS_thickness,OriginalSteelTemp\n";
                
    std::ofstream detailsOut("stack_details.csv");
    detailsOut << "slice,l/L,OriginalTPS,CarbonFiber_thickness,"
                << "Glue_thickness,Steel_thickness,"
                << "PreCarbonTemp,PreGlueTemp,PreSteelTemp,"
                << "OptimizedTPS,PostCarbonTemp,PostGlueTemp,PostSteelTemp\n";

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

        // ---- Stack setup (incl. grid gen) ----
        auto stack_start = Clock::now();
        Stack s;
        s.id = slice + 1;
        s.layers = {
            {{"TPS",         0.2,  160.0, 1200.0,   0.0, 1200.0}, matProps.getTPSThickness(lL),          pointsPerLayer},
            {{"CarbonFiber", 500.0,1600.0, 700.0,   0.0,  350.0}, matProps.getCarbonFiberThickness(lL),     pointsPerLayer},
            {{"Glue",        200.0,1300.0, 900.0,   0.0,  400.0}, matProps.getGlueThickness(lL),            pointsPerLayer},
            {{"Steel",       100.0,7850.0, 500.0, 800.0,    0.0}, matProps.getSteelThickness(lL),           pointsPerLayer}
        };
        matProps.generateGrid(s, pointsPerLayer);
        tStackSetup += MS(Clock::now() - stack_start).count();
    
        // compute interface indices once
        double tpsThick   = matProps.getTPSThickness(lL);
        double cfThick    = matProps.getCarbonFiberThickness(lL);
        double glueThick  = matProps.getGlueThickness(lL);
        double steelThick = matProps.getSteelThickness(lL);
        double posCG = tpsThick + cfThick;
        double posGS = posCG + glueThick;
        auto idxCarbonGlue = std::lower_bound(s.xGrid.begin(), s.xGrid.end(), posCG) - s.xGrid.begin();
        auto idxGlueSteel  = std::lower_bound(s.xGrid.begin(), s.xGrid.end(), posGS) - s.xGrid.begin();
        
    
        // ---- Original solver run ----
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
        

        // open time‐history file for this slice, original thickness
        std::ostringstream histOrigBuffer;
        histOrigBuffer << "time[s],T_carbon_glue[K],T_glue_steel[K],T_steel[K]\n";
        
        // Timer for solver per slice
        auto solve_start = Clock::now();
        // Time-marching loop
        while (!solver.isFinished()) {
            solver.step();
            double t = solver.getCurrentTime();
            const auto& Tdist = solver.getTemperatureDistribution();
            histOrigBuffer 
            << t                  << ","
            << Tdist[idxCarbonGlue]   << ","
            << Tdist[idxGlueSteel]    << ","
            << Tdist.back()           << "\n";
        }
        tOrigSolve += MS(Clock::now() - solve_start).count();

        // ---- Original history CSV save ----
        auto saveOrig_start = Clock::now();
        {
            std::ofstream histOrig(
                "time_history_orig_slice_" + std::to_string(slice+1) + ".csv"
            );
            histOrig << histOrigBuffer.str();
        }
        tHistOrigSave += MS(Clock::now() - saveOrig_start).count();
        
        // sample original temps
        const auto& Tdist = solver.getTemperatureDistribution();
        double steelT = Tdist.back();
        double origTempCarbon = Tdist[idxCarbonGlue];
        double origTempGlue   = Tdist[idxGlueSteel];
        double origTempSteel  = steelT;

        // Suggest TPS thickness (optional optimization)
        auto optSuggest_start = Clock::now();
        TemperatureComparator comp;
        comp.setTimeStep(cli.getTimeStep(), cli.useAdaptiveTimeStep());
        comp.setGridResolution(cli.getPointsPerLayer());
        // double tpsOpt = comp.suggestTPSThickness(s, 800.0, tFinal, lL, matProps, theta);
        double tpsOpt = comp.suggestTPSThickness(
                s,
                800.0,   // max steel temp @ steel/glue
                400.0,   // max glue  temp @ glue/carbon
                350.0,   // max carbon temp @ carbon/external
                tFinal,
                lL,
                matProps,
                theta
            );
        tOptSuggestion += MS(Clock::now() - optSuggest_start).count();

        // --- NEW: re-run solver at optimized thickness ---
        s.layers[0].thickness = tpsOpt;
        matProps.generateGrid(s, pointsPerLayer);

        TimeHandler th2(tFinal, dt, adapt);
        HeatEquationSolver solverOpt(theta);
        solverOpt.initialize(s, th2);
        solverOpt.setInitialTemperature(uniformInit.empty()
            ? std::vector<double>(s.xGrid.size(), 300.0)
            : uniformInit);
        solverOpt.setBoundaryConditions(
            new DirichletCondition(static_cast<float>(matProps.getExhaustTemp(lL))),
            new NeumannCondition(0.0f)
        );

        auto solveOpt_start = Clock::now();
        std::ostringstream histOptBuffer;
        histOptBuffer << "time[s],T_carbon_glue[K],T_glue_steel[K],T_steel[K]\n";
        
        while (!solverOpt.isFinished()) {
            solverOpt.step();
            double t2 = solverOpt.getCurrentTime();
            const auto& T2 = solverOpt.getTemperatureDistribution();
            histOptBuffer
              << t2                 << ","
              << T2[idxCarbonGlue]  << ","
              << T2[idxGlueSteel]   << ","
              << T2.back()          << "\n";
        }
        tOptSolve += MS(Clock::now() - solveOpt_start).count();
            
        // ---- Optimized history CSV save ----
        auto saveOpt_start = Clock::now();
        {
            std::ofstream histOpt(
                "time_history_opt_slice_" + std::to_string(slice+1) + ".csv"
            );
            histOpt << histOptBuffer.str();
        }
        tHistOptSave += MS(Clock::now() - saveOpt_start).count();



        // sample optimized steel temp
        double steelOpt = solverOpt.getTemperatureDistribution().back();
        const auto& Topt = solverOpt.getTemperatureDistribution();
        double postTempCarbon = Topt[idxCarbonGlue];
        double postTempGlue   = Topt[idxGlueSteel];
        double postTempSteel  = steelOpt;

        // ---- Summary & details CSV writes ----
        auto out_start = Clock::now();

        // Write summary CSV with the optimized‐thickness temperature
        summaryOut 
        << (slice+1) << ","
        << lL         << ","
        << "BTCS"     << ","
        << steelOpt   << ","
        << tpsOpt     << ","
        << steelT     << "\n";

        // write the detailed interface temps
        detailsOut
          << (slice+1) << ","
          << lL         << ","
          << tpsThick   << ","  // original TPS
          << cfThick    << ","
          << glueThick  << ","
          << steelThick << ","
          << origTempCarbon << ","
          << origTempGlue   << ","
          << origTempSteel  << ","
          << tpsOpt         << ","
          << postTempCarbon << ","
          << postTempGlue   << ","
          << postTempSteel  << "\n";

          tSummaryDetailsWrite += MS(Clock::now() - out_start).count();
    } // end for slices

    summaryOut.close();
    detailsOut.close();

    // overall end
    double overallMs = MS(Clock::now() - overall_start).count();
    
    // === PRINT ALL TIMES ===
    std::cout << "\n=== Timers ===\n";
    std::cout << "Mesh load time:               " << tMeshLoad           << " ms\n";
    std::cout << "Init temp load time:          " << tInitTempLoad       << " ms\n";
    std::cout << "Stack setup time (total):     " << tStackSetup         << " ms\n";
    std::cout << "Original solver time:         " << tOrigSolve          << " ms\n";
    std::cout << "Orig. history CSV save:       " << tHistOrigSave       << " ms\n";
    std::cout << "Optimized solver time:        " << tOptSolve           << " ms\n";
    std::cout << "TPS-opt suggestion time:      " << tOptSuggestion      << " ms\n";
    std::cout << "Opt. history CSV save:        " << tHistOptSave        << " ms\n";
    std::cout << "Summary/details CSV writes:   " << tSummaryDetailsWrite<< " ms\n";
    std::cout << "Overall program time:         " << overallMs           << " ms\n";

    return 0;
}
