/**
 * @file AdaptiveMeshGenerator.h
 * @brief Header file for the AdaptiveMeshGenerator class.
 *
 * This file defines the AdaptiveMeshGenerator class, which provides functionality
 * to generate adaptive volume meshes by computing the Boolean difference between
 * a cube and an inner mesh. The resulting mesh can be saved to a file.
 */

 #ifndef ADAPTIVE_MESH_GENERATOR_H
 #define ADAPTIVE_MESH_GENERATOR_H
 
 #include <string>
 #include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
 #include <CGAL/Surface_mesh.h>
 #include <CGAL/Mesh_polyhedron_3.h>
 #include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
 #include <CGAL/boost/graph/copy_face_graph.h>
 
 namespace params = CGAL::parameters;
 
 /**
  * @class AdaptiveMeshGenerator
  * @brief A class for generating adaptive volume meshes.
  *
  * This class provides functionality to generate a volume mesh by computing the Boolean difference
  * between a cube and an inner mesh. The resulting mesh can be saved to a file.
  */
 class AdaptiveMeshGenerator {
 public:
     /**
      * @typedef Kernel
      * @brief The kernel type used in the meshing process.
      */
     typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
     
     /**
      * @typedef Point
      * @brief The point type used in the meshing process.
      */
     typedef Kernel::Point_3 Point;
     
     /**
      * @typedef SurfaceMesh
      * @brief The surface mesh type used in the meshing process.
      */
     typedef CGAL::Surface_mesh<Point> SurfaceMesh;
     
     /**
      * @typedef Polyhedron
      * @brief The polyhedron type used in the meshing process.
      */
     typedef CGAL::Mesh_polyhedron_3<Kernel>::type Polyhedron;
     
     /**
      * @brief Constructs an AdaptiveMeshGenerator object.
      */
     AdaptiveMeshGenerator();
 
     /**
      * @brief Generates the volume mesh by computing the Boolean difference (cube \ inner).
      *
      * @param innerMeshFile Path to the inner mesh OFF file.
      * @param cubeSize Half-size of the cube (cube extends from -cubeSize to cubeSize).
      * @param outputMeshFile Filename for the output mesh (default is "adaptive_mesh.off").
      * @return Returns true if the mesh is generated successfully.
      */
     bool generateVolumeMesh(const std::string &innerMeshFile,
                             int cubeSize,
                             const std::string &outputMeshFile = "adaptive_mesh.off");
 };
 
 #endif // ADAPTIVE_MESH_GENERATOR_H
 