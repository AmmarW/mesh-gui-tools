/**
 * @file MeshValidator.h
 * @brief Defines the MeshValidator class for validating mesh structures.
 *
 * This file provides the MeshValidator class, which includes static methods to
 * validate various aspects of a mesh, such as checking for closed surfaces and
 * consistent face orientations.
 */

 #ifndef MESHVALIDATOR_H
 #define MESHVALIDATOR_H
 
 #include "Mesh.h"
 #include <vector>
 #include <string>
 
 /**
  * @class MeshValidator
  * @brief A utility class for validating mesh structures.
  *
  * The MeshValidator class provides static methods to validate various aspects
  * of a mesh, such as checking for closed surfaces, consistent face orientations,
  * and structural integrity.
  */
 class MeshValidator {
 public:
     /**
      * @brief Validates the mesh for closed surfaces, consistent face orientations, and other issues.
      *
      * This static method checks the provided mesh for common issues such as:
      * - Closed surfaces
      * - Consistent face orientations
      * - Other structural integrity checks
      *
      * @param mesh The mesh to be validated.
      * @return A vector of error messages if issues are found, otherwise an empty vector.
      */
     static std::vector<std::string> validate(const Mesh& mesh);
 };
 
 #endif // MESHVALIDATOR_H
 