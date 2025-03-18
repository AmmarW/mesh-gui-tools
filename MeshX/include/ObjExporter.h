/**
 * @file ObjExporter.h
 * @brief Defines the ObjExporter class for exporting meshes to OBJ files.
 *
 * This file provides the ObjExporter class, which includes functionality to
 * export a given mesh to an OBJ file format.
 */

 #ifndef OBJEXPORTER_H
 #define OBJEXPORTER_H
 
 #include "Mesh.h"
 #include <string>
 
 /**
  * @class ObjExporter
  * @brief A utility class for exporting meshes to OBJ files.
  *
  * This class provides a static method to export a given mesh to an OBJ file.
  */
 class ObjExporter {
 public:
     /**
      * @brief Exports the given mesh to an OBJ file.
      *
      * This function takes a mesh and a file path, and exports the mesh to the specified
      * file in the OBJ format.
      *
      * @param mesh The mesh to be exported.
      * @param filePath The path to the file where the mesh will be exported.
      * @return True if the export is successful, false otherwise.
      */
     static bool exportMesh(const Mesh& mesh, const std::string& filePath);
 };
 
 #endif // OBJEXPORTER_H
 