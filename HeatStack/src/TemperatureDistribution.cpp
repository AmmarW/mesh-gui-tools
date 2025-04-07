#include "TemperatureDistribution.h"

TemperatureDistribution::TemperatureDistribution() {}

TemperatureDistribution::~TemperatureDistribution() {}

void TemperatureDistribution::initialize(int size, double defaultValue) {
    temperatures.resize(size, defaultValue);
}

std::vector<double>& TemperatureDistribution::data() {
    return temperatures;
}

const std::vector<double>& TemperatureDistribution::data() const {
    return temperatures;
}
