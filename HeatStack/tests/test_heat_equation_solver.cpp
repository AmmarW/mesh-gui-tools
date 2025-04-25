#include "../include/HeatEquationSolver.h"
#include "../include/MaterialProperties.h"
#include "../include/BoundaryConditions.h"
#include <cassert>
#include <iostream>

void testHeatEquationSolver() {
    MaterialProperties props;
    Stack stack = props.getStack(1);
    TimeHandler timeHandler(10.0, 0.1, true);
    HeatEquationSolver solver(0.5); // Crank-Nicolson
    solver.initialize(stack, timeHandler);
    std::vector<double> initialTemp(stack.xGrid.size(), 300.0);
    solver.setInitialTemperature(initialTemp);
    solver.setBoundaryConditions(new DirichletCondition(300.0), new NeumannCondition(0.0));
    solver.step();
    const auto& temps = solver.getTemperatureDistribution();
    assert(temps.size() == stack.xGrid.size() && "Temperature vector size mismatch");
    std::cout << "HeatEquationSolver test passed.\n";
}

int main() {
    testHeatEquationSolver();
    return 0;
}