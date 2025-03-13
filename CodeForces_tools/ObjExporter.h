/**
 * @class ObjExporter
 * @brief A utility class for exporting meshes to OBJ files.
 *
 * This class provides a static method to export a given mesh to an OBJ file.
 */

/**
 * @brief Exports the given mesh to an OBJ file.
 *
 * This function takes a mesh and a file path, and exports the mesh to the specified
 * file in the OBJ format.
 *
 * @param mesh The mesh to be exported.
 * @param filePath The path to the file where the mesh will be exported.
 * @return true if the export is successful, false otherwise.
 */
#ifndef OBJEXPORTER_H
#define OBJEXPORTER_H

#include "Mesh.h"
#include <string>

class ObjExporter {
public:
    // Exports the given mesh to an OBJ file.
    // Returns true on success, false otherwise.
    static bool exportMesh(const Mesh& mesh, const std::string& filePath);
};

#endif // OBJEXPORTER_H
