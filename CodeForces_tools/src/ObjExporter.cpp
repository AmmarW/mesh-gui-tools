
/**
 * @file ObjExporter.cpp
 * @brief Implementation of the ObjExporter class for exporting mesh data to OBJ files.
 */

#include "ObjExporter.h"
#include <fstream>

/**
 * @brief Exports the given mesh to an OBJ file at the specified file path.
 * 
 * This function writes the vertices, texture coordinates, normals, and faces of the mesh
 * to an OBJ file. The OBJ file format is a simple data-format that represents 3D geometry.
 * 
 * @param mesh The mesh to be exported.
 * @param filePath The path to the file where the mesh will be exported.
 * @return true if the mesh was successfully exported, false otherwise.
 */

bool ObjExporter::exportMesh(const Mesh& mesh, const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    // Write vertices.
    for (const auto& vertex : mesh.vertices) {
        file << "v " << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
    }
    
    // Write texture coordinates, if available.
    for (const auto& tex : mesh.texCoords) {
        file << "vt " << tex[0] << " " << tex[1] << "\n";
    }
    
    // Write normals, if available.
    for (const auto& normal : mesh.normals) {
        file << "vn " << normal.x << " " << normal.y << " " << normal.z << "\n";
    }
    
    // Write faces.
    for (const auto& face : mesh.faces) {
        file << "f";
        for (const auto& fe : face.elements) {
            file << " " << (fe.vertexIndex + 1);
            // Write texture and normal indices if available.
            if (fe.texCoordIndex != -1 || fe.normalIndex != -1) {
                file << "/";
                if (fe.texCoordIndex != -1) {
                    file << (fe.texCoordIndex + 1);
                }
                if (fe.normalIndex != -1) {
                    file << "/" << (fe.normalIndex + 1);
                }
            }
        }
        file << "\n";
    }
    
    return true;
}
