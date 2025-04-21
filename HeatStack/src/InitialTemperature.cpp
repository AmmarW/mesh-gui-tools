#include "InitialTemperature.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

InitialTemperature::InitialTemperature() {}
InitialTemperature::~InitialTemperature() {}

std::vector<double> InitialTemperature::loadInitialTemperature(const std::string &source) {
    std::vector<double> temperatures;
    std::ifstream file(source);

    if (!file.is_open()) {
        throw std::runtime_error("Error: Unable to open file " + source);
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip header lines starting with '#'
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        int id;
        double temperature;

        // Parse ID and temperature from the line
        if (ss >> id && ss.ignore(1, ',') && ss >> temperature) {
            temperatures.push_back(temperature);
        } else {
            throw std::runtime_error("Error: Invalid format in file " + source);
        }
    }

    file.close();
    return temperatures;
}

std::vector<double> InitialTemperature::createUniformDistribution(int size, double value) {
    return std::vector<double>(size, value);
}