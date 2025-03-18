/**
 * @file MeshConverter.h
 * @brief Defines the MeshConverter class for converting between different mesh formats.
 *
 * This file provides the MeshConverter class, which includes functions for converting
 * OBJ files to OFF files, OFF files to OBJ files, and converting a custom Mesh object
 * into a CGAL Polyhedron representation.
 */

 #ifndef MESHCONVERTER_H
 #define MESHCONVERTER_H
 
 #include <string>
 #include <CGAL/Simple_cartesian.h>
 #include <CGAL/Surface_mesh.h>
 #include "Mesh.h"
 #include "MeshBooleanOperations.h"
 
 /**
  * @typedef Kernel
  * @brief Defines the CGAL kernel type for geometric operations.
  */
 typedef CGAL::Simple_cartesian<double> Kernel;
 
 /**
  * @typedef Point
  * @brief Represents a 3D point in CGAL operations.
  */
 typedef Kernel::Point_3 Point;
 
 /**
  * @typedef CGALMesh
  * @brief Defines a CGAL surface mesh type, renamed to avoid conflicts with custom Mesh.h.
  */
 typedef CGAL::Surface_mesh<Point> CGALMesh;
 
 /**
  * @class MeshConverter
  * @brief Provides functionality for converting between different mesh formats.
  */
 class MeshConverter {
 public:
     /**
      * @brief Constructs a MeshConverter object.
      */
     MeshConverter();
 
     /**
      * @brief Converts an OBJ file to an OFF file.
      *
      * Reads a mesh from an OBJ file and writes it to an OFF file.
      *
      * @param input_filename The path to the input OBJ file.
      * @param output_filename The path to the output OFF file.
      * @return True if the conversion is successful, false otherwise.
      */
     bool convertObjToOff(const std::string& input_filename, const std::string& output_filename);
 
     /**
      * @brief Converts an OFF file to an OBJ file.
      *
      * Reads a mesh from an OFF file and writes it to an OBJ file.
      *
      * @param offFile The path to the input OFF file.
      * @param objFile The path to the output OBJ file.
      * @return True if the conversion is successful, false otherwise.
      */
     bool convertOffToObj(const std::string& offFile, const std::string& objFile);
     
     /**
      * @brief Converts a custom Mesh object into a CGAL Polyhedron representation.
      *
      * This function converts a given Mesh object into a CGAL Polyhedron (OFF format)
      * that can be used for Boolean operations or other CGAL processing.
      *
      * @param mesh The input custom Mesh object.
      * @param poly Reference to the output CGAL Polyhedron.
      * @return True if the conversion is successful, false otherwise.
      */
     bool convertMeshToPolyhedron(const Mesh &mesh, MeshBooleanOperations::Polyhedron &poly);
 };
 
 #endif // MESHCONVERTER_H
 