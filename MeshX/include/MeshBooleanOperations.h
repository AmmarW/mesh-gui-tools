/**
 * @file MeshBooleanOperations.h
 * @brief Defines a class for performing Boolean operations on 3D meshes.
 *
 * This file contains the MeshBooleanOperations class, which provides functions
 * for reading, writing, and performing Boolean operations (union, intersection,
 * and difference) on 3D polyhedral meshes.
 */

 #ifndef MESH_BOOLEAN_OPERATIONS_H
 #define MESH_BOOLEAN_OPERATIONS_H
 
 #include <string>
 #include <vector>
 #include <fstream>
 #include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
 #include <CGAL/Surface_mesh.h>
 #include <CGAL/Mesh_polyhedron_3.h>
 #include <CGAL/Polyhedral_mesh_domain_with_features_3.h>
 #include <CGAL/Sizing_field_with_aabb_tree.h>
 #include <CGAL/Polygon_mesh_processing/corefinement.h>
 #include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
 #include <CGAL/boost/graph/copy_face_graph.h>
 #include <CGAL/boost/graph/generators.h>
 #include <CGAL/Mesh_triangulation_3.h>
 #include <CGAL/Mesh_complex_3_in_triangulation_3.h>
 #include <CGAL/Mesh_criteria_3.h>
 #include <CGAL/make_mesh_3.h>
 
 namespace PMP = CGAL::Polygon_mesh_processing;
 
 /**
  * @class MeshBooleanOperations
  * @brief Provides Boolean operations on 3D polyhedral meshes.
  *
  * This class supports reading, writing, and performing Boolean operations such as
  * union, intersection, and difference on 3D polyhedral meshes.
  */
 class MeshBooleanOperations {
 public:
     /**
      * @typedef Kernel
      * @brief The kernel type used for geometric operations.
      */
     typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
     
     /**
      * @typedef Point
      * @brief Represents a 3D point in the mesh.
      */
     typedef Kernel::Point_3 Point;
     
     /**
      * @typedef Polyhedron
      * @brief The polyhedral mesh type used for Boolean operations.
      */
     typedef CGAL::Mesh_polyhedron_3<Kernel>::type Polyhedron;
 
     /**
      * @brief Reads an OFF file into a Polyhedron.
      * @param filename The path to the OFF file.
      * @param poly Reference to the Polyhedron where the mesh will be stored.
      * @return True if the file was successfully read, false otherwise.
      */
     static bool readOFF(const std::string &filename, Polyhedron &poly);
 
     /**
      * @brief Writes a Polyhedron to an OFF file.
      * @param filename The path to the output OFF file.
      * @param poly The Polyhedron to be written.
      * @return True if the file was successfully written, false otherwise.
      */
     static bool writeOFF(const std::string &filename, const Polyhedron &poly);
 
     /**
      * @brief Computes the union of multiple meshes.
      *
      * The operation is performed iteratively: result = mesh[0] ∪ mesh[1] ∪ ... ∪ mesh[n].
      *
      * @param meshes A vector of Polyhedrons representing the input meshes.
      * @param result Reference to store the resulting unioned mesh.
      * @return True if the operation is successful, false otherwise.
      */
     static bool computeUnion(const std::vector<Polyhedron> &meshes, Polyhedron &result);
 
     /**
      * @brief Computes the intersection of multiple meshes.
      *
      * The operation is performed iteratively: result = mesh[0] ∩ mesh[1] ∩ ... ∩ mesh[n].
      *
      * @param meshes A vector of Polyhedrons representing the input meshes.
      * @param result Reference to store the resulting intersected mesh.
      * @return True if the operation is successful, false otherwise.
      */
     static bool computeIntersection(const std::vector<Polyhedron> &meshes, Polyhedron &result);
 
     /**
      * @brief Computes the difference of multiple meshes.
      *
      * This operation subtracts the union of all meshes (except the first) from the first mesh:
      * result = mesh[0] \ (mesh[1] ∪ mesh[2] ∪ ...).
      *
      * @param meshes A vector of Polyhedrons representing the input meshes.
      * @param result Reference to store the resulting difference mesh.
      * @return True if the operation is successful, false otherwise.
      */
     static bool computeDifference(const std::vector<Polyhedron> &meshes, Polyhedron &result);
 };
 
 #endif // MESH_BOOLEAN_OPERATIONS_H
 