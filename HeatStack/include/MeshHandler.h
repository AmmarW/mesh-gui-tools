#ifndef MESH_HANDLER_H
#define MESH_HANDLER_H

#include <string>
#include <vector>
#include <array>

class MeshHandler {
public:
    MeshHandler();
    MeshHandler(const std::string& filename); // New constructor
    ~MeshHandler();

    // Loads a mesh from a .obj file
    bool loadMesh(const std::string &filename);

    // Access mesh data
    const std::vector<std::array<float, 3>>& getVertices() const;
    const std::vector<std::array<int, 3>>& getFaces() const;

    // Get mesh bounds (min/max values for each axis)
    float getMinZ() const;
    float getMaxZ() const;
    float getMinX() const;
    float getMaxX() const;
    float getMinY() const;
    float getMaxY() const;

private:
    std::vector<std::array<float, 3>> vertices;
    std::vector<std::array<int, 3>> faces;
};

#endif // MESH_HANDLER_H