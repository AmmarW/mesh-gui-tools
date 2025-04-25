#ifndef SAFETY_ARBITRATOR_H
#define SAFETY_ARBITRATOR_H

#include <vector>

// Handler for arbitrating solutions based on safe temperature criteria
class SafetyArbitrator {
public:
    SafetyArbitrator();
    ~SafetyArbitrator();

    // Evaluate safety criteria based on the current temperature distribution.
    // Evaluate if the steel temperature is below maxTemp
    bool evaluate(const std::vector<double>& temperatureDistribution, double maxTemp);

    // Placeholder: Weight or additional criteria can be added later.
private:
    // Placeholder: Criteria parameters for safety evaluation.
};

#endif // SAFETY_ARBITRATOR_H