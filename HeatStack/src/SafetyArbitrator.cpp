#include "SafetyArbitrator.h"

SafetyArbitrator::SafetyArbitrator() {}

SafetyArbitrator::~SafetyArbitrator() {}

bool SafetyArbitrator::evaluate(const std::vector<double>& temperatureDistribution, double maxTemp) {
    // Check the inner steel surface temperature
    double steelTemp = temperatureDistribution.back();
    return steelTemp < maxTemp;
}