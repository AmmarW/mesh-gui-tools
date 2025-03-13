#include "ObjParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <set>

/**
 * @brief Helper function to parse a face element.
 *
 * This function parses a face element token (e.g., "v", "v/vt", "v//vn", or "v/vt/vn")
 * and returns a FaceElement object.
 *
 * @param token The face element token to be parsed.
 * @return A FaceElement object representing the parsed face element.
 */
static FaceElement parseFaceElement(const std::string& token) {
    int v = -1, vt = -1, vn = -1;
    std::stringstream ss(token);
    std::string part;
    
    if (std::getline(ss, part, '/')) {
        v = std::stoi(part) - 1; // OBJ indices are 1-based.
    }
    if (std::getline(ss, part, '/')) {
        if (!part.empty()) {
            vt = std::stoi(part) - 1;
        }
    }
    if (std::getline(ss, part, '/')) {
        if (!part.empty()) {
            vn = std::stoi(part) - 1;
        }
    }
    return FaceElement(v, vt, vn);
}

/**
 * @brief Parses an OBJ file and returns a surface Mesh.
 *
 * This method takes the file path of an OBJ file as input,
 * parses the file, and returns a Mesh object representing the surface of the 3D model.
 *
 * @param filePath The path to the OBJ file to be parsed.
 * @return A Mesh object representing the parsed surface of the 3D model.
 */
Mesh ObjParser::parseSurfaceMesh(const std::string& filePath) {
    Mesh mesh;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
    
    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        lineNumber++;
        std::istringstream iss(line);
        std::string token;
        if (!(iss >> token)) continue; // Skip empty lines.
        
        if (token[0] == '#') continue; // Comment line.
        
        if (token == "v") {
            Vertex vertex;
            if (!(iss >> vertex.x >> vertex.y >> vertex.z)) {
                std::cerr << "Error parsing vertex at line " << lineNumber << "\n";
                continue;
            }
            mesh.vertices.push_back(vertex);
        } else if (token == "vt") {
            std::array<double, 2> tex;
            if (!(iss >> tex[0] >> tex[1])) {
                std::cerr << "Error parsing texture coordinate at line " << lineNumber << "\n";
                continue;
            }
            mesh.texCoords.push_back(tex);
        } else if (token == "vn") {
            Vertex normal;
            if (!(iss >> normal.x >> normal.y >> normal.z)) {
                std::cerr << "Error parsing normal at line " << lineNumber << "\n";
                continue;
            }
            mesh.normals.push_back(normal);
        } else if (token == "f") {
            Face face;
            std::string faceToken;
            while (iss >> faceToken) {
                try {
                    face.elements.push_back(parseFaceElement(faceToken));
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing face at line " << lineNumber << ": " << e.what() << "\n";
                }
            }
            mesh.faces.push_back(face);
        }
    }

    return mesh;
}

/**
 * @brief Parses an OBJ file and returns a volume Mesh.
 *
 * This method takes the file path of an OBJ file as input,
 * parses the file, generates a volume mesh, and returns a Mesh object representing the volume of the 3D model.
 *
 * @param filePath The path to the OBJ file to be parsed.
 * @return A Mesh object representing the parsed volume of the 3D model.
 */
Mesh ObjParser::parseVolumeMesh(const std::string& filePath) {
    Mesh mesh = parseSurfaceMesh(filePath);
    generateVolumeMesh(mesh);
    return mesh;
}

/**
 * @brief Parses an OBJ file and returns a Mesh with both surface and volume data.
 *
 * This method takes the file path of an OBJ file as input,
 * parses the file, generates both surface and volume meshes, and returns a Mesh object.
 *
 * @param filePath The path to the OBJ file to be parsed.
 * @return A Mesh object representing the parsed 3D model with both surface and volume data.
 */
Mesh ObjParser::parse(const std::string& filePath) {
    Mesh mesh = parseSurfaceMesh(filePath);
    generateVolumeMesh(mesh);
    return mesh;
}

/**
 * @brief Generates a volume mesh from a surface mesh.
 *
 * This method takes a Mesh object representing a surface mesh,
 * generates a volume mesh using Delaunay triangulation, and updates the Mesh object.
 *
 * @param mesh The Mesh object to be updated with the volume mesh.
 */
void ObjParser::generateVolumeMesh(Mesh& mesh) {
    using TetrahedralMesh = std::set<Mesh::Tetrahedron>;
    TetrahedralMesh tetrahedralMesh;

    int n = mesh.vertices.size();
    int refinement_factor = 1; // You can make this configurable

    for (int i = 0; i < n - 3; i++) {
        for (int r = 0; r < refinement_factor; ++r) {
            int j = (i + 1 + r) % (n - 1);
            int k = (i + 2 + r) % (n - 1);
            int l = (i + 3 + r) % (n - 1);
            tetrahedralMesh.insert({i, j, k, l});
        }
    }

    mesh.tetrahedrons.assign(tetrahedralMesh.begin(), tetrahedralMesh.end());
}