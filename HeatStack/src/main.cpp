#include "CLI.h"
#include "TimeHandler.h"
#include "InitialTemperature.h"
#include "TemperatureDistribution.h"
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
    InitialTemperature initialTempHandler;
    TemperatureDistribution tempDistHandler;

    try {
        // Load initial temperature data from the CSV file
        std::vector<double> temperatures = initialTempHandler.loadInitialTemperature("../src/initial_temperature.csv");

        // Initialize the temperature distribution with the loaded data
        tempDistHandler.initialize(temperatures.size(), 0.0); // Initialize with default value (0.0)
        tempDistHandler.update(temperatures); // Update with the loaded temperatures

        // Print the initialized temperature distribution
        std::cout << "Initialized Temperature Distribution:" << std::endl;
        for (double temp : tempDistHandler.data()) {
            std::cout << temp << " ";
        }
        std::cout << std::endl;

        // Access a specific temperature value
        int index = 2; // Example index
        std::cout << "Temperature at index " << index << ": " 
                  << tempDistHandler.getTemperatureAt(index) << std::endl;

        // Get a range of temperatures
        int start = 1, end = 3; // Example range
        std::vector<double> range = tempDistHandler.getTemperatureRange(start, end);
        std::cout << "Temperature range (" << start << " to " << end << "): ";
        for (double temp : range) {
            std::cout << temp << " ";
        }
        std::cout << std::endl;

        // Export the temperature distribution to a file
        tempDistHandler.exportToFile("exported_temperature_distribution.csv");
        std::cout << "Temperature distribution exported to 'build/exported_temperature_distribution.csv'." << std::endl;

    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
    std::cout << "\n[INFO] Simulation complete after " << timeHandler.getStepCount() << " steps.\n";
    return 0;
}