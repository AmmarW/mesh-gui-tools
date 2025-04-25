#ifndef BTCS_MATRIX_SOLVER_H
#define BTCS_MATRIX_SOLVER_H

#include <vector>

// Custom class for efficient BTCS method matrix operations using Thomas algorithm.
class BTCSMatrixSolver {
public:
    BTCSMatrixSolver();
    ~BTCSMatrixSolver();

    // Setup the solver matrix with the given size.
    void setupMatrix(int size);

    // Solve the tridiagonal matrix equation A * x = b using Thomas algorithm.
    std::vector<double> solve(const std::vector<double>& b);

private:
    int matrixSize;
    std::vector<double> a_; // Sub-diagonal
    std::vector<double> b_; // Main diagonal
    std::vector<double> c_; // Super-diagonal
};

#endif // BTCS_MATRIX_SOLVER_H