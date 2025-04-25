#include "BoundaryConditions.h"

/////////////////////////////////////////
// DirichletCondition: T = constant
/////////////////////////////////////////
DirichletCondition::DirichletCondition(float temperature) : temperature_(temperature) {}

DirichletCondition::~DirichletCondition() {}

BoundaryType DirichletCondition::getType() const {
    return BoundaryType::Dirichlet;
}

float DirichletCondition::getValue(const std::array<float, 3>& position) const {
    return temperature_;
}

/////////////////////////////////////////
// NeumannCondition: dT/dn = constant flux
/////////////////////////////////////////
NeumannCondition::NeumannCondition(float flux) : flux_(flux) {}

NeumannCondition::~NeumannCondition() {}

BoundaryType NeumannCondition::getType() const {
    return BoundaryType::Neumann;
}

float NeumannCondition::getValue(const std::array<float, 3>& position) const {
    return flux_;
}

