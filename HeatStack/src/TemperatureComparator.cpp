#include "TemperatureComparator.h"
#include "HeatEquationSolver.h"
#include "TimeHandler.h"
#include "BoundaryConditions.h"
#include <iostream>
#include <cmath>

TemperatureComparator::TemperatureComparator() {}

TemperatureComparator::~TemperatureComparator() {}

double TemperatureComparator::suggestTPSThickness(const Stack& stack, double maxTemp, double duration, double l_over_L, const MaterialProperties& props, double theta) {
    double minThickness = props.getMinTPSThickness();
    double maxThickness = props.getMaxTPSThickness();
    double tolerance = 0.00001; // 0.001 cm
    double thickness = minThickness;

    while (maxThickness - minThickness > tolerance) {
        thickness = (minThickness + maxThickness) / 2.0;
        Stack testStack = stack;
        testStack.layers[0].thickness = thickness;
        MaterialProperties tempProps;
        tempProps.generateGrid(testStack, compPoints);

        std::vector<double> temperatures = runSimulation(testStack, duration, theta, l_over_L);
        double steelTemp = temperatures.back();
        // std::cerr << "[cmp] steelTemp=" << steelTemp << "\n";
        if (steelTemp < maxTemp) {
            maxThickness = thickness;
        } else {
            minThickness = thickness;
        }
        
        // std::cerr << "[cmp] thickness=" << thickness
        // << "  dt=" << compDt
        // << "  pts/layer=" << compPoints
        // << "  run duration=" << duration << "\n";

    }
    return thickness;
}

std::vector<double> TemperatureComparator::runSimulation(Stack stack, double duration, double theta, double l_over_L) {
    TimeHandler timeHandler(duration, compDt, compAdapt);
    HeatEquationSolver solver(theta);
    solver.initialize(stack, timeHandler);

    // Set initial temperature (room temperature: 300K)
    std::vector<double> initialTemp(stack.xGrid.size(), 300.0);
    solver.setInitialTemperature(initialTemp);

    // Set boundary conditions (Dirichlet with exhaust gas temp, Neumann)
    double T_surface = -100 * std::log(8 * l_over_L + 1) + 900; // Exhaust gas temperature
    solver.setBoundaryConditions(new DirichletCondition(static_cast<float>(T_surface)), new NeumannCondition(0.0f));


    // Run simulation
    while (!solver.isFinished()) {
        solver.step();
        // timeHandler.advance();
    }
    return solver.getTemperatureDistribution();
}


void TemperatureComparator::setTimeStep(double dt, bool adaptive) {
    compDt = dt;
    compAdapt = adaptive;
}

void TemperatureComparator::setGridResolution(int pointsPerLayer) {
    compPoints = pointsPerLayer;
}