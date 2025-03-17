/**
 * @file MeshTransform.h
 * @brief Provides functions to apply transformations (translation, scaling, rotation) to a Mesh.
 *
 * This file contains the declaration of transformation functions that allow translation,
 * scaling, and rotation operations to be performed on a mesh.
 *
 * Usage:
 *  - Call `translate()`, `scale()`, or `rotate()` with the required parameters.
 */

 #ifndef MESHTRANSFORM_H
 #define MESHTRANSFORM_H
 
 #include "Mesh.h"
 
 class MeshTransform {
 public:
     /**
      * @brief Translates the mesh by given offsets.
      * @param mesh Reference to the Mesh object.
      * @param tx Translation along X-axis.
      * @param ty Translation along Y-axis.
      * @param tz Translation along Z-axis.
      */
     static void translate(Mesh& mesh, double tx, double ty, double tz);
 
     /**
      * @brief Scales the mesh by given factors.
      * @param mesh Reference to the Mesh object.
      * @param sx Scaling factor along X-axis.
      * @param sy Scaling factor along Y-axis.
      * @param sz Scaling factor along Z-axis.
      */
     static void scale(Mesh& mesh, double sx, double sy, double sz);
 
     /**
      * @brief Rotates the mesh by specified angles around the X, Y, and Z axes.
      * @param mesh Reference to the Mesh object.
      * @param angleX Rotation angle around X-axis (degrees).
      * @param angleY Rotation angle around Y-axis (degrees).
      * @param angleZ Rotation angle around Z-axis (degrees).
      */
     static void rotate(Mesh& mesh, double angleX, double angleY, double angleZ);
 };
 
 #endif // MESHTRANSFORM_H
 