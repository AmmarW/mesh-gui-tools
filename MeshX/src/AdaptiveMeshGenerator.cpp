/**
 * @file AdaptiveMeshGenerator.cpp
 * @brief Implements adaptive volume mesh generation using Boolean operations.
 */

 #include "AdaptiveMeshGenerator.h"
 #include <CGAL/Mesh_triangulation_3.h>
 #include <CGAL/Mesh_complex_3_in_triangulation_3.h>
 #include <CGAL/Mesh_criteria_3.h>
 #include <CGAL/make_mesh_3.h>
 #include <CGAL/Sizing_field_with_aabb_tree.h>
 #include <CGAL/Polyhedral_mesh_domain_with_features_3.h>
 #include <CGAL/Polygon_mesh_processing/corefinement.h>
 #include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
 #include <fstream>
 #include <iostream>
 #include <cstdlib>
 
 namespace PMP = CGAL::Polygon_mesh_processing;
 
 #ifdef CGAL_CONCURRENT_MESH_3
   typedef CGAL::Parallel_tag Concurrency_tag;
 #else
   typedef CGAL::Sequential_tag Concurrency_tag;
 #endif
 
 /**
  * @brief Default constructor for the AdaptiveMeshGenerator class.
  */
 AdaptiveMeshGenerator::AdaptiveMeshGenerator() {}
 
 /**
  * @brief Generates a volume mesh by computing the Boolean difference between a cube and an inner mesh.
  *
  * This function reads an inner mesh from a file, creates a cube of specified size, computes the Boolean difference
  * between the cube and the inner mesh, and generates a volume mesh based on the difference. The resulting mesh is
  * saved to an output file.
  *
  * @param innerMeshFile The file path to the inner mesh.
  * @param cubeSize The size of the cube (bounding volume) to be created.
  * @param outputMeshFile The file path where the output mesh will be saved.
  * @return True if the volume mesh generation is successful, false otherwise.
  */
 bool AdaptiveMeshGenerator::generateVolumeMesh(const std::string &innerMeshFile,
                                                  int cubeSize,
                                                  const std::string &outputMeshFile)
 {
     // 1. Read the inner object from file.
     SurfaceMesh inner;
     std::ifstream inner_in(innerMeshFile);
     if (!inner_in || !(inner_in >> inner)) {
         std::cerr << "Error: Cannot read file " << innerMeshFile << std::endl;
         return false;
     }
     if (!CGAL::is_closed(inner)) {
         std::cerr << "Error: Inner mesh is not closed." << std::endl;
         return false;
     }
     // Triangulate the inner mesh for robust Boolean operations.
     PMP::triangulate_faces(inner);
 
     // 2. Create a cube (outer bounding volume) with corners at (-cubeSize,...)
     SurfaceMesh cube;
     {
         Point p0(-cubeSize, -cubeSize, -cubeSize), p1(cubeSize, -cubeSize, -cubeSize);
         Point p2(cubeSize, cubeSize, -cubeSize),  p3(-cubeSize, cubeSize, -cubeSize);
         Point p4(-cubeSize, -cubeSize, cubeSize),  p5(cubeSize, -cubeSize, cubeSize);
         Point p6(cubeSize, cubeSize, cubeSize),     p7(-cubeSize, cubeSize, cubeSize);
         CGAL::make_hexahedron(p0, p1, p2, p3, p4, p5, p6, p7, cube);
     }
     if (!CGAL::is_closed(cube)) {
         std::cerr << "Error: Cube mesh is not closed." << std::endl;
         return false;
     }
     // Triangulate the cube (make_hexahedron produces quads by default)
     PMP::triangulate_faces(cube);
 
     // 3. Compute the Boolean difference: (cube \ inner)
     SurfaceMesh difference;
     std::cout << "Computing Boolean difference (cube \\ inner)..." << std::endl;
     bool diff_success = PMP::corefine_and_compute_difference(cube, inner, difference,
                               params::all_default(), params::all_default());
     if (!diff_success) {
         std::cerr << "Error: Boolean difference failed." << std::endl;
         return false;
     }
 
     // 4. Convert the difference (SurfaceMesh) to a Polyhedron.
     Polyhedron diff_poly;
     CGAL::copy_face_graph(difference, diff_poly);
     if (diff_poly.empty()) {
         std::cerr << "Error: Difference polyhedron is empty." << std::endl;
         return false;
     }
 
     // 5. Create a mesh domain with features from the difference polyhedron.
     typedef CGAL::Polyhedral_mesh_domain_with_features_3<Kernel, Polyhedron> Mesh_domain;
     Mesh_domain domain(diff_poly);
     // Optionally, you can call: domain.detect_features();
 
     // 6. Define an adaptive sizing field using CGAL's Sizing_field_with_aabb_tree.
     typedef CGAL::Sizing_field_with_aabb_tree<Kernel, Mesh_domain> Sizing_field;
     Sizing_field sizing_field(0.7, domain);
 
     // 7. Set up mesh criteria.
     typedef CGAL::Mesh_triangulation_3<Mesh_domain, CGAL::Default, Concurrency_tag>::type Tr;
     typedef CGAL::Mesh_complex_3_in_triangulation_3<Tr, Mesh_domain::Corner_index, Mesh_domain::Curve_index> C3t3;
     typedef CGAL::Mesh_criteria_3<Tr> Mesh_criteria;
     Mesh_criteria criteria(params::edge_size(sizing_field).
                            edge_distance(0.01).
                            facet_angle(25).
                            facet_size(sizing_field).
                            facet_distance(0.01).
                            cell_radius_edge_ratio(3).
                            cell_size(sizing_field));
 
     // 8. Generate the volume mesh.
     C3t3 c3t3 = CGAL::make_mesh_3<C3t3>(domain, criteria, params::no_exude().no_perturb());
     CGAL::dump_c3t3(c3t3, outputMeshFile.c_str());
 
     std::cout << "Mesh generation complete. Output written to " << outputMeshFile << std::endl;
     return true;
 }
 