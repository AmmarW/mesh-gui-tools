#include "CLI.h"
#include "TimeHandler.h"

#include <iostream>

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    CLI cli(argc, argv);
    if (cli.isHelpRequested()) {
        return 0;
    }

    // Print parsed input for debug/log
    std::cout << "[INFO] Starting HeatStack Simulation\n";
    std::cout << "Mesh file        : " << cli.getMeshFile() << "\n";
    std::cout << "Total time       : " << cli.getTimeDuration() << " s\n";
    std::cout << "Timestep         : " << cli.getTimeStep() << " s\n";
    std::cout << "Adaptive         : " << (cli.useAdaptiveTimeStep() ? "Enabled" : "Disabled") << "\n";
    std::cout << "Output file      : " << cli.getOutputFile() << "\n\n";

    // Initialize TimeHandler
    TimeHandler timeHandler(cli.getTimeDuration(), cli.getTimeStep(), cli.useAdaptiveTimeStep());

    // Main simulation loop
    while (!timeHandler.isFinished()) {
        double t = timeHandler.getCurrentTime();
        double dt = timeHandler.getTimeStep();

        std::cout << "[STEP " << timeHandler.getStepCount() << "] Time = " << t << "s, dt = " << dt << "\n";

        // TODO: Call solver step here
        // solver.solve(t, dt);

        // TODO: Check stability condition, adjust timestep if needed
        // if (unstable) timeHandler.adjustTimeStep(smaller_dt);

        timeHandler.advance();
    }

    std::cout << "\n[INFO] Simulation complete after " << timeHandler.getStepCount() << " steps.\n";
    return 0;
}