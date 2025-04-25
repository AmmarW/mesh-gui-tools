#include "BTCSMatrixSolver.h"
#include <stdexcept>

BTCSMatrixSolver::BTCSMatrixSolver() : matrixSize(0) {}

BTCSMatrixSolver::~BTCSMatrixSolver() {}

void BTCSMatrixSolver::setupMatrix(int size) {
    matrixSize = size;
    a_.resize(size - 1, 0.0); // Sub-diagonal
    b_.resize(size, 0.0);     // Main diagonal
    c_.resize(size - 1, 0.0); // Super-diagonal
}

std::vector<double> BTCSMatrixSolver::solve(const std::vector<double>& b) {
    if (b.size() != static_cast<size_t>(matrixSize)) {
        throw std::runtime_error("Size mismatch in BTCSMatrixSolver::solve");
    }

    // Thomas algorithm for tridiagonal matrix
    std::vector<double> c_prime(matrixSize - 1, 0.0);
    std::vector<double> d_prime(matrixSize, 0.0);
    std::vector<double> x(matrixSize, 0.0);

    // Forward elimination
    c_prime[0] = c_[0] / b_[0];
    d_prime[0] = b[0] / b_[0];
    for (int i = 1; i < matrixSize - 1; ++i) {
        double denom = b_[i] - a_[i - 1] * c_prime[i - 1];
        c_prime[i] = c_[i] / denom;
        d_prime[i] = (b[i] - a_[i - 1] * d_prime[i - 1]) / denom;
    }
    d_prime[matrixSize - 1] = (b[matrixSize - 1] - a_[matrixSize - 2] * d_prime[matrixSize - 2]) /
                              (b_[matrixSize - 1] - a_[matrixSize - 2] * c_prime[matrixSize - 2]);

    // Back substitution
    x[matrixSize - 1] = d_prime[matrixSize - 1];
    for (int i = matrixSize - 2; i >= 0; --i) {
        x[i] = d_prime[i] - c_prime[i] * x[i + 1];
    }

    return x;
}