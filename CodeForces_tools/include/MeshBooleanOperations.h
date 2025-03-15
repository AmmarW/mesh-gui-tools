#ifndef MESH_BOOLEAN_OPERATIONS_H
#define MESH_BOOLEAN_OPERATIONS_H

#include <string>
#include <vector>
#include <fstream>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Mesh_polyhedron_3.h>  // Recommended polyhedron type for meshing with features
#include <CGAL/Polyhedral_mesh_domain_with_features_3.h>
#include <CGAL/Sizing_field_with_aabb_tree.h>

#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/boost/graph/copy_face_graph.h>
#include <CGAL/boost/graph/generators.h>
#include <CGAL/Mesh_polyhedron_3.h>
#include <CGAL/Mesh_triangulation_3.h>
#include <CGAL/Mesh_complex_3_in_triangulation_3.h>
#include <CGAL/Mesh_criteria_3.h>
#include <CGAL/make_mesh_3.h>

namespace PMP = CGAL::Polygon_mesh_processing;

class MeshBooleanOperations {
public:
    // Define kernel and polyhedron types.
    typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
    typedef Kernel::Point_3 Point;
    typedef CGAL::Mesh_polyhedron_3<Kernel>::type Polyhedron;

    // Reads an OFF file into a Polyhedron.
    static bool readOFF(const std::string &filename, Polyhedron &poly);

    // Writes a Polyhedron to an OFF file.
    static bool writeOFF(const std::string &filename, const Polyhedron &poly);

    // Computes the union of a vector of meshes.
    // The operation is performed iteratively: result = mesh[0] ∪ mesh[1] ∪ ... ∪ mesh[n].
    static bool computeUnion(const std::vector<Polyhedron> &meshes, Polyhedron &result);

    // Computes the intersection of a vector of meshes.
    // The operation is performed iteratively.
    static bool computeIntersection(const std::vector<Polyhedron> &meshes, Polyhedron &result);

    // Computes the difference: subtracts the union of all meshes (except the first)
    // from the first mesh: result = mesh[0] \ (mesh[1] ∪ mesh[2] ∪ ...).
    static bool computeDifference(const std::vector<Polyhedron> &meshes, Polyhedron &result);
};

#endif // MESH_BOOLEAN_OPERATIONS_H
