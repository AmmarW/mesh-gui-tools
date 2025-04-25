#ifndef TEMPERATURE_DISTRIBUTION_H
#define TEMPERATURE_DISTRIBUTION_H

#include <vector>
#include <string>

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
    void update(const std::vector<double>& newTemperatures);
    double getTemperatureAt(int index) const;
    std::vector<double> getTemperatureRange(int start, int end) const;
    void exportToFile(const std::string& filename) const;

private:
    // Using std::vector here; can later be replaced with a custom container if needed.
    std::vector<double> temperatures;
};

#endif // TEMPERATURE_DISTRIBUTION_H