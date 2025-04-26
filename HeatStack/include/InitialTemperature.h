#ifndef INITIAL_TEMPERATURE_H
#define INITIAL_TEMPERATURE_H

#include <vector>
#include <string>

// Class to handle the initial temperature distribution.
class InitialTemperature {
public:
    InitialTemperature();
    InitialTemperature(const std::string& filePath); // New constructor
    ~InitialTemperature();

    // Load an initial temperature distribution from a file or another source.
    std::vector<double> loadInitialTemperature(const std::string &source);

    // Create a uniform temperature distribution.
    std::vector<double> createUniformDistribution(int size, double value);

    // Get the loaded temperature distribution.
    const std::vector<double>& getTemperatureDistribution() const;

private:
    std::vector<double> temperatureDistribution; // Store the temperature distribution
};

#endif // INITIAL_TEMPERATURE_H