#include "../include/BoundaryConditions.h"
#include <iostream>
#include <cassert>

void testDirichlet() {
    DirichletCondition dirichlet(100.0f);
    std::array<float, 3> pos = {0.0, 0.0, 0.0};
    assert(dirichlet.getType() == BoundaryType::Dirichlet);
    assert(dirichlet.getValue(pos) == 100.0f);
    std::cout << "Dirichlet test passed.\n";
}

void testNeumann() {
    NeumannCondition neumann(5.0f);
    std::array<float, 3> pos = {0.5, 0.5, 1.0};
    assert(neumann.getType() == BoundaryType::Neumann);
    assert(neumann.getValue(pos) == 5.0f);
    std::cout << "Neumann test passed.\n";
}

int main() {
    testDirichlet();
    testNeumann();
    std::cout << "All boundary condition tests passed.\n";
    return 0;
}