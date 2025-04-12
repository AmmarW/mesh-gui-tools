#include "CLI.h"
#include <iostream>
#include "InitialTemperature.h"
#include "TemperatureDistribution.h"

int main(int argc, char* argv[]) {
    // CLI cli;
    // cli.run();

    InitialTemperature initialTempHandler;
    TemperatureDistribution tempDistHandler;

    try {
        // Load initial temperature data from the CSV file
        std::vector<double> temperatures = initialTempHandler.loadInitialTemperature("initial_temperature.csv");

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
        std::cout << "Temperature distribution exported to 'exported_temperature_distribution.csv'." << std::endl;

    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}