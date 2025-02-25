/**
 * Validates the given mesh for various conditions and returns a list of error messages.
 *
 * @param mesh The mesh to be validated.
 * @return A vector of strings containing error messages, if any.
 *
 * The validation checks include:
 * - Closed surfaces: Ensures every edge is shared by exactly two faces.
 *   - If an edge is not shared by exactly two faces, an error message is generated.
 * - Additional validations (placeholders for future implementation):
 *   - Consistent face orientation: Compute and compare normals.
 *   - Self-intersections: Implement spatial partitioning/intersection tests.
 */
#include "MeshValidator.h"
#include <map>
#include <sstream>
#include <algorithm>

std::vector<std::string> MeshValidator::validate(const Mesh& mesh) {
    std::vector<std::string> errors;
    
    // Check for closed surfaces: every edge should be shared by exactly two faces.
    std::map<std::pair<int, int>, int> edgeCount;
    for (const auto& face : mesh.faces) {
        size_t numElements = face.elements.size();
        for (size_t i = 0; i < numElements; ++i) {
            int v1 = face.elements[i].vertexIndex;
            int v2 = face.elements[(i + 1) % numElements].vertexIndex;
            if (v1 > v2) std::swap(v1, v2);
            std::pair<int, int> edge(v1, v2);
            edgeCount[edge]++;
        }
    }
    
    for (const auto& entry : edgeCount) {
        if (entry.second != 2) {
            std::stringstream ss;
            ss << "Edge (" << entry.first.first << ", " << entry.first.second << ") appears " 
               << entry.second << " times. Expected 2 for a closed surface.";
            errors.push_back(ss.str());
        }
    }
    
    // Placeholders for additional validations:
    // - Consistent face orientation: compute and compare normals.
    // - Self-intersections: implement spatial partitioning/intersection tests.
    
    return errors;
}
