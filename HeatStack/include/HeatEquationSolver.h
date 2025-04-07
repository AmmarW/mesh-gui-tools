#ifndef HEAT_EQUATION_SOLVER_H
#define HEAT_EQUATION_SOLVER_H

#include <vector>
#include "BTCSMatrixSolver.h"
#include "TimeHandler.h"

// Solver for the 1D heat equation using the BTCS method.
class HeatEquationSolver {
public:
    HeatEquationSolver();
    ~HeatEquationSolver();

    // Initialize the solver with the problem size and time handling.
    void initialize(int size, const TimeHandler &timeHandler);

    // Set the initial temperature distribution.
    void setInitialTemperature(const std::vector<double>& initialTemp);

    // Advance the simulation by one time step.
    void step();

    // Retrieve the current temperature distribution.
    const std::vector<double>& getTemperatureDistribution() const;

private:
    int problemSize;
    std::vector<double> temperature;
    BTCSMatrixSolver matrixSolver;
    TimeHandler timeHandler;

    // Placeholder: Function to apply boundary conditions.
    void applyBoundaryConditions();
};

#endif // HEAT_EQUATION_SOLVER_H
