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
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            float x, y, z;
            ss >> x >> y >> z;
            vertices.push_back({x, y, z});
        } else if (type == "f") {
            std::vector<int> indices;
            std::string token;
            while (ss >> token) {
                std::istringstream ts(token);
                int index;
                ts >> index;
                indices.push_back(index - 1); // Convert OBJ 1-based to 0-based
            }

            // Fan triangulation: (a, b, c), (a, c, d), ...
            for (size_t i = 1; i + 1 < indices.size(); ++i) {
                faces.push_back({indices[0], indices[i], indices[i + 1]});
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
    float minZ = std::numeric_limits<float>::max();
    for (const auto& v : vertices) {
        if (v[2] < minZ) minZ = v[2];
    }
    return minZ;
}

float MeshHandler::getMaxZ() const {
    float maxZ = std::numeric_limits<float>::lowest();
    for (const auto& v : vertices) {
        if (v[2] > maxZ) maxZ = v[2];
    }
    return maxZ;
}