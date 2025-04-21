#include "TemperatureDistribution.h"
#include <fstream>
#include <stdexcept>
#include <sstream>

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

void TemperatureDistribution::update(const std::vector<double>& newTemperatures) {
    if (newTemperatures.size() != temperatures.size()) {
        throw std::invalid_argument("Error: Size of new temperature data does not match the current distribution size.");
    }
    temperatures = newTemperatures;
}

double TemperatureDistribution::getTemperatureAt(int index) const {
    if (index < 0 || index >= static_cast<int>(temperatures.size())) {
        throw std::out_of_range("Error: Index out of range.");
    }
    return temperatures[index];
}

std::vector<double> TemperatureDistribution::getTemperatureRange(int start, int end) const {
    if (start < 0 || end >= static_cast<int>(temperatures.size()) || start > end) {
        throw std::out_of_range("Error: Invalid range specified.");
    }
    return std::vector<double>(temperatures.begin() + start, temperatures.begin() + end + 1);
}

void TemperatureDistribution::exportToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Unable to open file " + filename);
    }

    for (size_t i = 0; i < temperatures.size(); ++i) {
        file << temperatures[i];
        if (i != temperatures.size() - 1) {
            file << ",";
        }
    }
    file.close();
}