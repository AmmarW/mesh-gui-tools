#include "MeshHandler.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <limits>

MeshHandler::MeshHandler() {}

MeshHandler::MeshHandler(const std::string& filename) {
    if (!loadMesh(filename)) {
        std::cerr << "Error: Failed to load mesh from file: " << filename << std::endl;
    }
}

MeshHandler::~MeshHandler() {}

bool MeshHandler::loadMesh(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open mesh file: " << filename << std::endl;
        return false;
    }

    vertices.clear();
    faces.clear();

    std::string line;
    size_t lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        if (line.empty() || line[0] == '#') 
            continue;  // skip comments and blank lines

        std::istringstream ss(line);
        std::string type;
        if (!(ss >> type)) {
            std::cerr << "Warning: Empty or invalid line at " << lineNumber << std::endl;
            continue;
        }

        if (type == "v") {
            float x, y, z;
            if (!(ss >> x >> y >> z)) {
                std::cerr << "Error: Invalid vertex format at line " << lineNumber
                          << " (expected 3 floats): \"" << line << "\"" << std::endl;
                return false;
            }
            vertices.push_back({x, y, z});

        } else if (type == "f") {
            std::vector<int> indices;
            std::string token;
            while (ss >> token) {
                // OBJ face tokens can be "v", "v/vt", "v/vt/vn"
                size_t slashPos = token.find('/');
                std::string idxStr = (slashPos == std::string::npos)
                                         ? token
                                         : token.substr(0, slashPos);
                try {
                    int idx = std::stoi(idxStr) - 1;  // 1‐based → 0‐based
                    indices.push_back(idx);
                } catch (const std::exception&) {
                    std::cerr << "Error: Invalid face index at line " << lineNumber
                              << ": \"" << token << "\"" << std::endl;
                    return false;
                }
            }

            if (indices.size() < 3) {
                std::cerr << "Warning: Face with fewer than 3 vertices at line "
                          << lineNumber << std::endl;
                continue;
            }

            // Fan triangulation
            for (size_t i = 1; i + 1 < indices.size(); ++i) {
                faces.push_back({indices[0], indices[i], indices[i + 1]});
            }

        } else {
            // // Unknown line type: skip or warn
            // std::cerr << "Warning: Unrecognized line type \"" << type
            //           << "\" at line " << lineNumber << std::endl;
        }
    }

    // Final validation
    if (vertices.empty()) {
        std::cerr << "Error: No vertices loaded from file: " << filename << std::endl;
        return false;
    }
    for (size_t f = 0; f < faces.size(); ++f) {
        for (int idx : faces[f]) {
            if (idx < 0 || idx >= static_cast<int>(vertices.size())) {
                std::cerr << "Error: Face " << f << " has invalid vertex index "
                          << idx << " (vertex count: " << vertices.size() << ")" << std::endl;
                return false;
            }
        }
    }

    return true;
}

const std::vector<std::array<float, 3>>& MeshHandler::getVertices() const {
    return vertices;
}

const std::vector<std::array<int, 3>>& MeshHandler::getFaces() const {
    return faces;
}

float MeshHandler::getMinZ() const {
    if (vertices.empty()) {
        std::cerr << "Error: Cannot compute getMinZ()—no vertices loaded." << std::endl;
        return 0.0f;
    }
    float minZ = std::numeric_limits<float>::max();
    for (const auto& v : vertices) {
        minZ = std::min(minZ, v[2]);
    }
    return minZ;
}

float MeshHandler::getMaxZ() const {
    if (vertices.empty()) {
        std::cerr << "Error: Cannot compute getMaxZ()—no vertices loaded." << std::endl;
        return 0.0f;
    }
    float maxZ = std::numeric_limits<float>::lowest();
    for (const auto& v : vertices) {
        maxZ = std::max(maxZ, v[2]);
    }
    return maxZ;
}
