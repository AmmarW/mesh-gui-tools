#ifndef BOUNDARY_CONDITIONS_H
#define BOUNDARY_CONDITIONS_H

#include <array>

enum class BoundaryType {
    Dirichlet,
    Neumann
};

class BoundaryCondition {
public:
    virtual ~BoundaryCondition() {}
    virtual BoundaryType getType() const = 0;
    virtual float getValue(const std::array<float, 3>& position) const = 0;
};

// Dirichlet: fixed temperature
class DirichletCondition : public BoundaryCondition {
public:
    DirichletCondition(float temperature);
    ~DirichletCondition() override;

    BoundaryType getType() const override;
    float getValue(const std::array<float, 3>& position) const override;

private:
    float temperature_;
};

// Neumann: fixed heat flux
class NeumannCondition : public BoundaryCondition {
public:
    NeumannCondition(float flux);
    ~NeumannCondition() override;

    BoundaryType getType() const override;
    float getValue(const std::array<float, 3>& position) const override;

private:
    float flux_;
};


#endif // BOUNDARY_CONDITIONS_H