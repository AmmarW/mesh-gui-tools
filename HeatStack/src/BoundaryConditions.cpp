#include "BoundaryConditions.h"

DirichletCondition::DirichletCondition() {}
DirichletCondition::~DirichletCondition() {}
BoundaryType DirichletCondition::getType() const { return BoundaryType::Dirichlet; }

NeumannCondition::NeumannCondition() {}
NeumannCondition::~NeumannCondition() {}
BoundaryType NeumannCondition::getType() const { return BoundaryType::Neumann; }

RobinCondition::RobinCondition() {}
RobinCondition::~RobinCondition() {}
BoundaryType RobinCondition::getType() const { return BoundaryType::Robin; }
