#ifndef HEAT_EQUATION_SOLVER_H
#define HEAT_EQUATION_SOLVER_H

#include <vector>
#include "BTCSMatrixSolver.h"
#include "TimeHandler.h"
#include "MaterialProperties.h"
#include "BoundaryConditions.h"

// Solver for the 1D heat equation using the θ-method (BTCS when θ=1, Crank-Nicolson when θ=0.5)
class HeatEquationSolver {
public:
    HeatEquationSolver(double theta = 0.5);
    ~HeatEquationSolver();

    // Initialize the solver with stack properties and time handling
    void initialize(const Stack& stack, const TimeHandler& timeHandler);

    // Set the initial temperature distribution
    void setInitialTemperature(const std::vector<double>& initialTemp);

    // Set boundary conditions
    void setBoundaryConditions(BoundaryCondition* outerBC, BoundaryCondition* innerBC);

    // Advance the simulation by one time step
    void step();

    // Retrieve the current temperature distribution
    const std::vector<double>& getTemperatureDistribution() const;

    // Adjust time step based on CFL or error criteria
    void adjustTimeStep(double errorThreshold = 1e-3);

    // Return whether the solver’s own timeHandler_ has reached totalTime
    bool isFinished() const;

    // Get the current time from the time handler
    double getCurrentTime() const;

private:
    double theta_;                      // θ parameter (1 for BTCS, 0.5 for Crank-Nicolson)
    int problemSize_;                   // Number of grid points
    std::vector<double> temperature_;   // Current temperature distribution
    std::vector<double> prevTemperature_; // Previous time step for error estimation
    Stack stack_;                       // Material stack properties
    BTCSMatrixSolver matrixSolver_;     // Matrix solver (Thomas algorithm)
    TimeHandler timeHandler_;           // Time stepping control
    BoundaryCondition* outerBC_;        // Outer boundary condition (Dirichlet)
    BoundaryCondition* innerBC_;        // Inner boundary condition (Neumann)

    // Setup the tridiagonal matrix A for AX = B
    void setupMatrix(std::vector<std::vector<double>>& A, double dt);

    // Setup the right-hand side vector B
    void setupRHS(std::vector<double>& b, double dt);

    // Apply boundary conditions to the matrix and RHS
    void applyBoundaryConditions(std::vector<std::vector<double>>& A, std::vector<double>& b, double dt);

    // Compute thermal diffusivity for a grid point
    double getThermalDiffusivity(int i) const;

    // Estimate error for adaptive time stepping (Crank-Nicolson)
    double estimateError(double dt);
};

#endif // HEAT_EQUATION_SOLVER_H