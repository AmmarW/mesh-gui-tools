/**
 * @class MeshValidator
 * @brief A utility class for validating mesh structures.
 *
 * The MeshValidator class provides static methods to validate various aspects
 * of a mesh, such as checking for closed surfaces and consistent face orientations.
 */

/**
 * @brief Validates the mesh for closed surfaces, consistent face orientations, etc.
 *
 * This static method checks the provided mesh for common issues such as:
 * - Closed surfaces
 * - Consistent face orientations
 * - Other structural integrity checks
 *
 * @param mesh The mesh to be validated.
 * @return A vector of error messages if issues are found, otherwise an empty vector.
 */
#ifndef MESHVALIDATOR_H
#define MESHVALIDATOR_H

#include "Mesh.h"
#include <vector>
#include <string>

class MeshValidator {
public:
    // Validates the mesh for closed surfaces, consistent face orientations, etc.
    // Returns a vector of error messages if issues are found.
    static std::vector<std::string> validate(const Mesh& mesh);
};

#endif // MESHVALIDATOR_H
