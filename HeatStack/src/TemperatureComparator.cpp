#include "TemperatureComparator.h"
#include "HeatEquationSolver.h"
#include "TimeHandler.h"
#include "BoundaryConditions.h"
#include <iostream>
#include <cmath>

TemperatureComparator::TemperatureComparator() {}

TemperatureComparator::~TemperatureComparator() {}

double TemperatureComparator::suggestTPSThickness(const Stack& stack, double maxTemp, double duration, double l_over_L, const MaterialProperties& props) {
    double minThickness = props.getMinTPSThickness();
    double maxThickness = props.getMaxTPSThickness();
    double tolerance = 0.00001; // 0.001 cm
    double thickness = minThickness;

    while (maxThickness - minThickness > tolerance) {
        thickness = (minThickness + maxThickness) / 2.0;
        Stack testStack = stack;
        testStack.layers[0].thickness = thickness;
        MaterialProperties tempProps;
        tempProps.generateGrid(testStack);

        std::vector<double> temperatures = runSimulation(testStack, duration, 0.5, l_over_L);
        double steelTemp = temperatures.back();

        if (steelTemp < maxTemp) {
            maxThickness = thickness;
        } else {
            minThickness = thickness;
        }
    }
    return thickness;
}

std::vector<double> TemperatureComparator::runSimulation(Stack stack, double duration, double theta, double l_over_L) {
    TimeHandler timeHandler(duration, 0.1, true);
    HeatEquationSolver solver(theta);
    solver.initialize(stack, timeHandler);

    // Set initial temperature (room temperature: 300K)
    std::vector<double> initialTemp(stack.xGrid.size(), 300.0);
    solver.setInitialTemperature(initialTemp);

    // Set boundary conditions (Dirichlet with exhaust gas temp, Neumann)
    double T_surface = -100 * std::log(8 * l_over_L + 1) + 900; // Exhaust gas temperature
    solver.setBoundaryConditions(new DirichletCondition(static_cast<float>(T_surface)), new NeumannCondition(0.0f));

    // Run simulation
    while (!timeHandler.isFinished()) {
        solver.step();
        timeHandler.advance();
    }
    return solver.getTemperatureDistribution();
}