#include "InitialTemperature.h"

InitialTemperature::InitialTemperature() {}
InitialTemperature::~InitialTemperature() {}

std::vector<double> InitialTemperature::loadInitialTemperature(const std::string &source) {
    // Placeholder: Implement file reading or other initialization logic.
    return std::vector<double>();
}

std::vector<double> InitialTemperature::createUniformDistribution(int size, double value) {
    return std::vector<double>(size, value);
}
