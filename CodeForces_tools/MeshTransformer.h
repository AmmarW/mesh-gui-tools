/**
 * @file MeshTransformer.h
 * @brief Provides functions to apply transformations to a Mesh.
 */

#ifndef MESHTRANSFORMER_H
#define MESHTRANSFORMER_H

#include "Mesh.h"
#include <Eigen/Dense>

class MeshTransformer {
public:
    // Apply translation to the mesh
    static void translate(Mesh& mesh, double tx, double ty, double tz);

    // Apply scaling to the mesh
    static void scale(Mesh& mesh, double sx, double sy, double sz);

    // Apply rotation to the mesh (angle in degrees, axis: 'x', 'y', 'z')
    static void rotate(Mesh& mesh, double angle, char axis);
};

#endif // MESHTRANSFORMER_H
