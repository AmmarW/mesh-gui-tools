#ifndef MESH_HANDLER_H
#define MESH_HANDLER_H

#include <string>

class MeshHandler {
public:
    MeshHandler();
    ~MeshHandler();

    // Placeholder: Load mesh from a file (.obj or another format)
    bool loadMesh(const std::string &filename);

    // Additional mesh-related methods can be added later.
};

#endif // MESH_HANDLER_H
