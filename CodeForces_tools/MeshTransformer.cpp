/**
 * @file MeshTransformer.cpp
 * @brief Implementation of MeshTransformer class for applying transformations to a Mesh.
 */

#include "MeshTransformer.h"
#include <cmath>

// Function to apply translation
void MeshTransformer::translate(Mesh& mesh, double tx, double ty, double tz) {
    for (auto& vertex : mesh.vertices) {
        vertex.x += tx;
        vertex.y += ty;
        vertex.z += tz;
    }
}

// Function to apply scaling
void MeshTransformer::scale(Mesh& mesh, double sx, double sy, double sz) {
    for (auto& vertex : mesh.vertices) {
        vertex.x *= sx;
        vertex.y *= sy;
        vertex.z *= sz;
    }
}

// Function to apply rotation
void MeshTransformer::rotate(Mesh& mesh, double angle, char axis) {
    Eigen::Matrix3d rotationMatrix = Eigen::Matrix3d::Identity();
    double radians = angle * M_PI / 180.0;
    double cosA = cos(radians), sinA = sin(radians);

    switch (axis) {
        case 'x': // Rotation around X-axis
            rotationMatrix << 1, 0, 0,
                              0, cosA, -sinA,
                              0, sinA, cosA;
            break;
        case 'y': // Rotation around Y-axis
            rotationMatrix << cosA, 0, sinA,
                              0, 1, 0,
                              -sinA, 0, cosA;
            break;
        case 'z': // Rotation around Z-axis
            rotationMatrix << cosA, -sinA, 0,
                              sinA, cosA, 0,
                              0, 0, 1;
            break;
        default:
            throw std::invalid_argument("Invalid rotation axis. Use 'x', 'y', or 'z'.");
    }

    for (auto& vertex : mesh.vertices) {
        Eigen::Vector3d v(vertex.x, vertex.y, vertex.z);
        Eigen::Vector3d transformed = rotationMatrix * v;
        vertex.x = transformed.x();
        vertex.y = transformed.y();
        vertex.z = transformed.z();
    }
}
