#ifndef BOUNDARY_CONDITIONS_H
#define BOUNDARY_CONDITIONS_H

#include <array>

enum class BoundaryType {
    Dirichlet,
    Neumann,
    Robin
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

// Robin: heat exchange (h * (T_ext - T))
class RobinCondition : public BoundaryCondition {
public:
    RobinCondition(float h, float externalTemp);
    ~RobinCondition() override;

    BoundaryType getType() const override;
    float getValue(const std::array<float, 3>& position) const override;

private:
    float h_;
    float externalTemp_;
};

#endif // BOUNDARY_CONDITIONS_H
