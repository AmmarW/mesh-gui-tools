#ifndef INITIAL_TEMPERATURE_H
#define INITIAL_TEMPERATURE_H

#include <vector>
#include <string>

// Class to handle the initial temperature distribution.
class InitialTemperature {
public:
    InitialTemperature();
    ~InitialTemperature();

    // Load an initial temperature distribution from a file or another source (placeholder).
    std::vector<double> loadInitialTemperature(const std::string &source);

    // Create a uniform temperature distribution.
    std::vector<double> createUniformDistribution(int size, double value);

    // Placeholder: Additional methods for non-uniform distributions.
};

#endif // INITIAL_TEMPERATURE_H
