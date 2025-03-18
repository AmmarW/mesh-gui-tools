/**
 * @file MeshBooleanOperations.cpp
 * @brief Implements Boolean operations on polygonal meshes.
 */

#include "MeshBooleanOperations.h"

/**
 * @brief Reads a mesh from an OFF file.
 * @param filename Path to the OFF file.
 * @param poly Reference to the Polyhedron object to store the mesh.
 * @return True if the file is successfully read, false otherwise.
 */
bool MeshBooleanOperations::readOFF(const std::string &filename, Polyhedron &poly) {
    std::ifstream in(filename);
    if (!in || !(in >> poly)) {
        std::cerr << "Error: Cannot read OFF file: " << filename << std::endl;
        return false;
    }
    // Ensure the mesh is triangulated for robust Boolean operations.
    PMP::triangulate_faces(poly);
    return true;
}

/**
 * @brief Writes a mesh to an OFF file.
 * @param filename Path to the output OFF file.
 * @param poly The Polyhedron object representing the mesh.
 * @return True if the file is successfully written, false otherwise.
 */
bool MeshBooleanOperations::writeOFF(const std::string &filename, const Polyhedron &poly) {
    std::ofstream out(filename);
    if (!out) {
        std::cerr << "Error: Cannot write OFF file: " << filename << std::endl;
        return false;
    }
    out << poly;
    return true;
}

/**
 * @brief Computes the union of multiple meshes.
 * @param meshes Vector of Polyhedron objects representing input meshes.
 * @param result Reference to store the resulting unioned mesh.
 * @return True if the operation is successful, false otherwise.
 */
bool MeshBooleanOperations::computeUnion(const std::vector<Polyhedron> &meshes, Polyhedron &result) {
    if (meshes.empty()) {
        std::cerr << "Error: No meshes provided for union operation." << std::endl;
        return false;
    }
    result = meshes[0];
    for (size_t i = 1; i < meshes.size(); ++i) {
        Polyhedron temp;
        bool success = PMP::corefine_and_compute_union(
            result, 
            const_cast<Polyhedron&>(meshes[i]), 
            temp, 
            PMP::parameters::all_default(), 
            PMP::parameters::all_default(), 
            PMP::parameters::all_default()
        );
        if (!success) {
            std::cerr << "Error: Union operation failed between meshes." << std::endl;
            return false;
        }
        result = temp;
    }
    return true;
}

/**
 * @brief Computes the intersection of multiple meshes.
 * @param meshes Vector of Polyhedron objects representing input meshes.
 * @param result Reference to store the resulting intersected mesh.
 * @return True if the operation is successful, false otherwise.
 */
bool MeshBooleanOperations::computeIntersection(const std::vector<Polyhedron> &meshes, Polyhedron &result) {
    if (meshes.empty()) {
        std::cerr << "Error: No meshes provided for intersection operation." << std::endl;
        return false;
    }
    result = meshes[0];
    for (size_t i = 1; i < meshes.size(); ++i) {
        Polyhedron temp;
        bool success = PMP::corefine_and_compute_intersection(
            result, 
            const_cast<Polyhedron&>(meshes[i]), 
            temp
        );
        if (!success) {
            std::cerr << "Error: Intersection operation failed between meshes." << std::endl;
            return false;
        }
        result = temp;
    }
    return true;
}

/**
 * @brief Computes the difference of multiple meshes.
 * @param meshes Vector of Polyhedron objects representing input meshes.
 * @param result Reference to store the resulting difference mesh.
 * @return True if the operation is successful, false otherwise.
 */
bool MeshBooleanOperations::computeDifference(const std::vector<Polyhedron> &meshes, Polyhedron &result) {
    if (meshes.empty()) {
        std::cerr << "Error: No meshes provided for difference operation." << std::endl;
        return false;
    }
    if (meshes.size() == 1) {
        result = meshes[0];
        return true;
    }
    
    // First, compute the union of all meshes except the first.
    Polyhedron unionOther = meshes[1];
    for (size_t i = 2; i < meshes.size(); ++i) {
        Polyhedron temp;
        bool success = PMP::corefine_and_compute_union(
            unionOther, 
            const_cast<Polyhedron&>(meshes[i]), 
            temp,
            PMP::parameters::all_default(), 
            PMP::parameters::all_default(), 
            PMP::parameters::all_default()
        );
        if (!success) {
            std::cerr << "Error: Union operation failed during difference computation." << std::endl;
            return false;
        }
        unionOther = temp;
    }
    
    // Create a non-const copy of the first mesh
    Polyhedron mesh0 = meshes[0];
    
    // Compute the difference: mesh0 minus the union of the others.
    bool diffSuccess = PMP::corefine_and_compute_difference(
        mesh0, 
        unionOther, 
        result,
        PMP::parameters::all_default(), 
        PMP::parameters::all_default(), 
        PMP::parameters::all_default()
    );
    if (!diffSuccess) {
        std::cerr << "Error: Difference operation failed." << std::endl;
    }
    return diffSuccess;
}