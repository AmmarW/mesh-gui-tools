#include "MaterialProperties.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <stdexcept>

MaterialProperties::MaterialProperties() {
    // Initialize predefined materials based on properties.m
    Material tps = {"TPS", 0.2, 160, 1200, 0, 1200};
    Material carbonFiber = {"CarbonFiber", 500, 1600, 700, 0, 350};
    Material glue = {"Glue", 200, 1300, 900, 0, 400};
    Material steel = {"Steel", 100, 7850, 500, 800, 0};

    // Example stack (will be replaced by loadStacks or dynamic assignment)
    Stack stack;
    stack.id = 1;
    stack.layers = {
        {tps, 0.001, 10},         // TPS, 0.1 cm (to be optimized)
        {carbonFiber, 0.001, 10}, // Carbon-fiber, 0.1 cm (example)
        {glue, 0.001, 10},        // Glue, 0.1 cm
        {steel, 0.001, 10}        // Steel, 0.1 cm
    };
    stack.totalThickness = 0.004;
    generateGrid(stack);
    stacks.push_back(stack);
}

MaterialProperties::~MaterialProperties() {}

void MaterialProperties::loadStacks(const std::string& filename) {
    // Placeholder: Read stack configurations from a file
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open stack file: " + filename);
    }
    file.close();
}

Stack MaterialProperties::getStack(int id) const {
    for (const auto& stack : stacks) {
        if (stack.id == id) return stack;
    }
    throw std::runtime_error("Stack ID not found: " + std::to_string(id));
}

void MaterialProperties::generateGrid(Stack& stack, int pointsPerLayer) {
    stack.xGrid.clear();
    double x = 0.0;
    stack.xGrid.push_back(x);

    for (auto& layer : stack.layers) {
        layer.numPoints = pointsPerLayer;
        double dx = layer.thickness / (pointsPerLayer - 1);
        for (int i = 1; i < pointsPerLayer; ++i) {
            x += dx;
            stack.xGrid.push_back(x);
        }
    }
    stack.totalThickness = x;
}

double MaterialProperties::getTPSThickness(double l_over_L) const {
    return 0.001; // Placeholder, optimized in TemperatureComparator
}

double MaterialProperties::getCarbonFiberThickness(double l_over_L) const {
    double A = 0.015; // 1.5 cm
    double f = 1.0;   // 1 Hz
    double t = l_over_L * 2.5;
    return (std::abs(A * std::sin(2 * M_PI * f * t)) + 0.001) / 100.0; // cm to m
}

double MaterialProperties::getGlueThickness(double l_over_L) const {
    double A = 0.001; // 0.1 cm
    double B = 20.0;
    double C = 0.0001; // 0.01 cm
    return (A * std::log(B * l_over_L + 1) + C) / 100.0; // cm to m
}

double MaterialProperties::getSteelThickness(double l_over_L) const {
    double A = 0.05;  // 5 cm
    double f = 5.0;   // 5 Hz
    double t = l_over_L * 2.5;
    double sawtooth = 2 * (f * t - std::floor(f * t)) - 1;
    return ((A / 2) * (sawtooth + 1) + 0.001) / 100.0; // cm to m
}