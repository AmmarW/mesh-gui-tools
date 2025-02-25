/**
 * @file ObjParser.cpp
 * @brief Implementation of the ObjParser class for parsing .obj files into Mesh objects.
 * 
 * This file contains the implementation of the ObjParser class, which provides functionality
 * to parse .obj files and convert them into Mesh objects. The .obj file format is commonly
 * used for representing 3D geometry.
 * 
 * The parser supports the following elements:
 * - Vertex positions (v)
 * - Texture coordinates (vt)
 * - Vertex normals (vn)
 * - Faces (f)
 * 
 * The parser also handles comments (lines starting with '#') and skips empty lines.
 * 
 * The parseFaceElement function is a helper function used to parse face elements, which can
 * be in the form of "v", "v/vt", "v//vn", or "v/vt/vn".
 * 
 * The parse function reads the .obj file line by line, parses the relevant elements, and
 * populates the Mesh object with vertices, texture coordinates, normals, and faces.
 * 
 * If an error occurs during parsing (e.g., invalid format), an error message is printed to
 * std::cerr, and the parser continues to the next line.
 * 
 * @throws std::runtime_error if the file cannot be opened.
 * 
 * @param filePath The path to the .obj file to be parsed.
 * @return A Mesh object containing the parsed data.
 */
#include "ObjParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>

// Helper function to parse a face element (e.g., "v", "v/vt", "v//vn", or "v/vt/vn").
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

Mesh ObjParser::parse(const std::string& filePath) {
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
        // Extend here for additional tokens or data if needed.
    }
    return mesh;
}
