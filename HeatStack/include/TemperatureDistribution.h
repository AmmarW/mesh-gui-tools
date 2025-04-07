#ifndef TEMPERATURE_DISTRIBUTION_H
#define TEMPERATURE_DISTRIBUTION_H

#include <vector>

// A memory-efficient container for temperature distributions.
class TemperatureDistribution {
public:
    TemperatureDistribution();
    ~TemperatureDistribution();

    // Initialize the distribution with a given size and default value.
    void initialize(int size, double defaultValue = 0.0);

    // Access the underlying data (modifiable).
    std::vector<double>& data();

    // Access the underlying data (read-only).
    const std::vector<double>& data() const;

private:
    // Using std::vector here; can later be replaced with a custom container if needed.
    std::vector<double> temperatures;
};

#endif // TEMPERATURE_DISTRIBUTION_H
