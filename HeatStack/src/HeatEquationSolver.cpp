#include "HeatEquationSolver.h"
#include <iostream>

HeatEquationSolver::HeatEquationSolver() : problemSize(0) {}

HeatEquationSolver::~HeatEquationSolver() {}

void HeatEquationSolver::initialize(int size, const TimeHandler &tHandler) {
    problemSize = size;
    timeHandler = tHandler;
    temperature.resize(problemSize, 0.0);
    matrixSolver.setupMatrix(problemSize);
}

void HeatEquationSolver::setInitialTemperature(const std::vector<double>& initialTemp) {
    if(initialTemp.size() != static_cast<size_t>(problemSize)) {
        throw std::runtime_error("Initial temperature vector size does not match problem size.");
    }
    temperature = initialTemp;
}

void HeatEquationSolver::step() {
    // Apply boundary conditions (placeholder)
    applyBoundaryConditions();

    // Create the right-hand side vector for the BTCS method (currently the temperature vector)
    std::vector<double> rhs = temperature;

    // Solve the system
    temperature = matrixSolver.solve(rhs);

    // Placeholder: Adjust time step adaptively if needed.
}

const std::vector<double>& HeatEquationSolver::getTemperatureDistribution() const {
    return temperature;
}

void HeatEquationSolver::applyBoundaryConditions() {
    // Placeholder: Apply Dirichlet, Neumann, or Robin boundary conditions.
}
