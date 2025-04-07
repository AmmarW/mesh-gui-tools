#ifndef TEMPERATURE_COMPARATOR_H
#define TEMPERATURE_COMPARATOR_H

#include <vector>

// Class for comparing temperature distributions to suggest insulation.
class TemperatureComparator {
public:
    TemperatureComparator();
    ~TemperatureComparator();

    // Compare the given temperature distribution and suggest insulation criteria (placeholder).
    void compare(const std::vector<double>& temperatureDistribution);

    // Placeholder: Methods to store or output recommendations.
};

#endif // TEMPERATURE_COMPARATOR_H
