#include "../include/MeshHandler.h"
#include <iostream>
#include <cassert>

void testHumanoidMesh() {
    MeshHandler mesh;
    bool loaded = mesh.loadMesh("tests/humanoid_robot.obj");
    assert(loaded && "Failed to load humanoid_robot.obj");

    const auto& vertices = mesh.getVertices();
    const auto& faces = mesh.getFaces();

    std::cout << "Loaded " << vertices.size() << " vertices\n";
    std::cout << "Loaded " << faces.size() << " triangle faces (after triangulation)\n";

    assert(vertices.size() > 0 && "Vertex count should be greater than 0");
    assert(faces.size() > 0 && "Face count should be greater than 0");

    float minZ = mesh.getMinZ();
    float maxZ = mesh.getMaxZ();

    std::cout << "Z-Range: [" << minZ << ", " << maxZ << "]\n";
    assert(minZ < maxZ && "Min Z should be less than Max Z");

    std::cout << "Humanoid MeshHandler test passed.\n";
}

int main() {
    testHumanoidMesh();
    return 0;
}
