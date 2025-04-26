#include "InitialTemperature.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

InitialTemperature::InitialTemperature() {}

InitialTemperature::InitialTemperature(const std::string& filePath) {
    temperatureDistribution = loadInitialTemperature(filePath);
}

InitialTemperature::~InitialTemperature() {}

std::vector<double> InitialTemperature::loadInitialTemperature(const std::string &source) {
    std::ifstream file(source);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open initial temperature file: " + source);
    }

    std::vector<double> temperatures;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        double temp;
        while (ss >> temp) {
            temperatures.push_back(temp);
        }
    }

    if (temperatures.empty()) {
        throw std::runtime_error("Initial temperature file is empty or invalid: " + source);
    }

    return temperatures;
}

std::vector<double> InitialTemperature::createUniformDistribution(int size, double value) {
    return std::vector<double>(size, value);
}

const std::vector<double>& InitialTemperature::getTemperatureDistribution() const {
    return temperatureDistribution;
}