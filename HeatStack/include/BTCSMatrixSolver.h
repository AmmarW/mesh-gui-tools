#ifndef BTCS_MATRIX_SOLVER_H
#define BTCS_MATRIX_SOLVER_H

#include <vector>

// Custom class for efficient BTCS method matrix operations.
class BTCSMatrixSolver {
public:
    BTCSMatrixSolver();
    ~BTCSMatrixSolver();

    // Setup the solver matrix with the given size.
    void setupMatrix(int size);

    // Solve the matrix equation A * x = b.
    // This is a placeholder for a custom, efficient inversion or solver method.
    std::vector<double> solve(const std::vector<double>& b);

private:
    int matrixSize;
    // Internal matrix representation. This can later be replaced with a more optimized structure.
    std::vector<std::vector<double>> A;
};

#endif // BTCS_MATRIX_SOLVER_H
