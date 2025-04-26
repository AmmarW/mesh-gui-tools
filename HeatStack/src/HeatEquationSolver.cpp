#include "HeatEquationSolver.h"
#include "utils.h"
#include <stdexcept>
#include <cmath>


HeatEquationSolver::HeatEquationSolver(double theta) 
    : theta_(theta), problemSize_(0), outerBC_(nullptr), innerBC_(nullptr), timeHandler_(0.0, 1.0, false) {}

HeatEquationSolver::~HeatEquationSolver() {
    delete outerBC_;
    delete innerBC_;
}

void HeatEquationSolver::initialize(const Stack& stack, const TimeHandler& timeHandler) {
    stack_ = stack;
    problemSize_ = static_cast<int>(stack.xGrid.size()); // Explicit cast to int
    timeHandler_ = timeHandler;
    temperature_.resize(problemSize_, 0.0);
    prevTemperature_.resize(problemSize_, 0.0);
    matrixSolver_.setupMatrix(problemSize_);
}

void HeatEquationSolver::setInitialTemperature(const std::vector<double>& initialTemp) {
    if (initialTemp.size() != static_cast<size_t>(problemSize_)) {
        throw std::runtime_error("Initial temperature vector size does not match problem size.");
    }
    temperature_ = initialTemp;
    prevTemperature_ = initialTemp;
}

void HeatEquationSolver::setBoundaryConditions(BoundaryCondition* outerBC, BoundaryCondition* innerBC) {
    outerBC_ = outerBC;
    innerBC_ = innerBC;
}

void HeatEquationSolver::step() {
    double dt = timeHandler_.getTimeStep();
    std::vector<std::vector<double>> A(problemSize_, std::vector<double>(problemSize_, 0.0));
    std::vector<double> b(problemSize_, 0.0);

    // Setup matrix A and RHS b
    setupMatrix(A, dt);
    setupRHS(b, dt);
    applyBoundaryConditions(A, b, dt);

    // Solve AX = B
    temperature_ = matrixSolver_.solve(b);

    // Update previous temperature for error estimation
    prevTemperature_ = temperature_;

    // Adjust time step if adaptive
    if (timeHandler_.isAdaptive()) {
        adjustTimeStep();
    }
}

const std::vector<double>& HeatEquationSolver::getTemperatureDistribution() const {
    return temperature_;
}

void HeatEquationSolver::setupMatrix(std::vector<std::vector<double>>& A, double dt) {
    for (int i = 1; i < problemSize_ - 1; ++i) {
        double dx_left = stack_.xGrid[i] - stack_.xGrid[i - 1];
        double dx_right = stack_.xGrid[i + 1] - stack_.xGrid[i];
        double dx_avg = (dx_left + dx_right) / 2.0;
        double alpha = getThermalDiffusivity(i);
        double r = alpha * dt / (dx_avg * dx_avg);

        A[i][i - 1] = -theta_ * r;
        A[i][i] = 1 + 2 * theta_ * r;
        A[i][i + 1] = -theta_ * r;
    }
}

void HeatEquationSolver::setupRHS(std::vector<double>& b, double dt) {
    for (int i = 1; i < problemSize_ - 1; ++i) {
        double dx_left = stack_.xGrid[i] - stack_.xGrid[i - 1];
        double dx_right = stack_.xGrid[i + 1] - stack_.xGrid[i];
        double dx_avg = (dx_left + dx_right) / 2.0;
        double alpha = getThermalDiffusivity(i);
        double r = alpha * dt / (dx_avg * dx_avg);

        b[i] = temperature_[i] + (1 - theta_) * r * (temperature_[i - 1] - 2 * temperature_[i] + temperature_[i + 1]);
    }
}

void HeatEquationSolver::applyBoundaryConditions(std::vector<std::vector<double>>& A, std::vector<double>& b, double dt) {
    // Outer boundary (Dirichlet: T = T_surface)
    if (outerBC_ && outerBC_->getType() == BoundaryType::Dirichlet) {
        auto* dirichlet = dynamic_cast<DirichletCondition*>(outerBC_);
        double T_surface = dirichlet->getValue({0, 0, 0});
        A[0][0] = 1.0;
        b[0] = T_surface;
    }

    // Inner boundary (Neumann: dT/dx = 0)
    if (innerBC_ && innerBC_->getType() == BoundaryType::Neumann) {
        double dx = stack_.xGrid[problemSize_ - 1] - stack_.xGrid[problemSize_ - 2];
        double alpha = getThermalDiffusivity(problemSize_ - 1);
        double r = alpha * dt / (dx * dx);

        A[problemSize_ - 1][problemSize_ - 2] = -2 * theta_ * r;
        A[problemSize_ - 1][problemSize_ - 1] = 1 + 2 * theta_ * r;
        b[problemSize_ - 1] = temperature_[problemSize_ - 1] + (1 - theta_) * r * (temperature_[problemSize_ - 2] - 2 * temperature_[problemSize_ - 1] + temperature_[problemSize_ - 2]);
    }
}

double HeatEquationSolver::getThermalDiffusivity(int i) const {
    double x = stack_.xGrid[i];
    double x_start = 0.0;
    for (const auto& layer : stack_.layers) {
        if (x <= x_start + layer.thickness) {
            return layer.material.k / (layer.material.rho * layer.material.c);
        }
        x_start += layer.thickness;
    }
    return stack_.layers.back().material.k / (stack_.layers.back().material.rho * stack_.layers.back().material.c);
}

void HeatEquationSolver::adjustTimeStep(double errorThreshold) {
    if (theta_ == 0.5) { // Crank-Nicolson
        double error = estimateError(timeHandler_.getTimeStep());
        double dt = timeHandler_.getTimeStep();
        if (error > errorThreshold) {
            timeHandler_.adjustTimeStep(dt / 2.0);
        } else if (error < errorThreshold / 2.0) {
            timeHandler_.adjustTimeStep(dt * 2.0);
        }
    } else { // BTCS
        double maxAlpha = 0.0;
        for (int i = 0; i < problemSize_; ++i) {
            double alpha = getThermalDiffusivity(i);
            if (alpha > maxAlpha) maxAlpha = alpha;
        }
        double dx_min = stack_.xGrid[1] - stack_.xGrid[0];
        for (int i = 1; i < problemSize_ - 1; ++i) {
            double dx = stack_.xGrid[i + 1] - stack_.xGrid[i];
            if (dx < dx_min) dx_min = dx;
        }
        double dt_max = 0.5 * dx_min * dx_min / maxAlpha;
        if (timeHandler_.getTimeStep() > dt_max) {
            timeHandler_.adjustTimeStep(dt_max);
        }
    }
}

double HeatEquationSolver::estimateError(double dt) {
    std::vector<double> temp_half = temperature_;
    double dt_half = dt / 2.0;
    std::vector<std::vector<double>> A(problemSize_, std::vector<double>(problemSize_, 0.0));
    std::vector<double> b(problemSize_, 0.0);

    // First half step
    setupMatrix(A, dt_half);
    setupRHS(b, dt_half);
    applyBoundaryConditions(A, b, dt_half);
    temp_half = matrixSolver_.solve(b);

    // Second half step
    temperature_ = temp_half;
    setupMatrix(A, dt_half);
    setupRHS(b, dt_half);
    applyBoundaryConditions(A, b, dt_half);
    temp_half = matrixSolver_.solve(b);

    // Compare with full step
    double error = 0.0;
    for (int i = 0; i < problemSize_; ++i) {
        error += std::pow(temp_half[i] - prevTemperature_[i], 2);
    }
    return std::sqrt(error / problemSize_);
}