#include "InitialTemperature.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

InitialTemperature::InitialTemperature() {}
InitialTemperature::~InitialTemperature() {}

std::vector<double> InitialTemperature::loadInitialTemperature(const std::string &source) {
    // Placeholder: File loading not used as per requirements
    throw std::runtime_error("File-based initial temperature loading disabled.");
}

std::vector<double> InitialTemperature::createUniformDistribution(int size, double value) {
    return std::vector<double>(size, value); // Default to 300K for room temperature
}