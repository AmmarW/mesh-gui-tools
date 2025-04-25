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

/////////////////////////////////////////
// RobinCondition: h * (T_ext - T)
/////////////////////////////////////////
RobinCondition::RobinCondition(float h, float externalTemp)
    : h_(h), externalTemp_(externalTemp) {}

RobinCondition::~RobinCondition() {}

BoundaryType RobinCondition::getType() const {
    return BoundaryType::Robin;
}

// Placeholder: returns h * T_ext (actual T unknown here)
float RobinCondition::getValue(const std::array<float, 3>& position) const {
    return h_ * externalTemp_;
}