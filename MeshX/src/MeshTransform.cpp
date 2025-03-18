/**
 * @file MeshTransform.cpp
 * @brief Implements functions for applying transformations (translation, scaling, rotation) to a Mesh.
 */

 #include "MeshTransform.h"
 #include <cmath>
 
 // Define M_PI if not defined
 #ifndef M_PI
 #define M_PI 3.14159265358979323846
 #endif
 
 /**
  * @brief Translates a mesh by a given offset.
  *
  * This function moves all vertices in the mesh by the specified amounts along the x, y, and z axes.
  *
  * @param mesh The mesh to be translated.
  * @param tx Translation along the x-axis.
  * @param ty Translation along the y-axis.
  * @param tz Translation along the z-axis.
  */
 void MeshTransform::translate(Mesh& mesh, double tx, double ty, double tz) {
     for (auto& vertex : mesh.vertices) {
         vertex.x += tx;
         vertex.y += ty;
         vertex.z += tz;
     }
 }
 
 /**
  * @brief Scales a mesh by given factors along each axis.
  *
  * This function scales all vertices in the mesh relative to the origin.
  *
  * @param mesh The mesh to be scaled.
  * @param sx Scaling factor along the x-axis.
  * @param sy Scaling factor along the y-axis.
  * @param sz Scaling factor along the z-axis.
  */
 void MeshTransform::scale(Mesh& mesh, double sx, double sy, double sz) {
     for (auto& vertex : mesh.vertices) {
         vertex.x *= sx;
         vertex.y *= sy;
         vertex.z *= sz;
     }
 }
 
 /**
  * @brief Rotates a mesh around the x, y, and z axes by specified angles.
  *
  * The function rotates all vertices in the mesh using rotation matrices for each axis.
  *
  * @param mesh The mesh to be rotated.
  * @param angleX Rotation angle (in degrees) around the x-axis.
  * @param angleY Rotation angle (in degrees) around the y-axis.
  * @param angleZ Rotation angle (in degrees) around the z-axis.
  */
 void MeshTransform::rotate(Mesh& mesh, double angleX, double angleY, double angleZ) {
     // Convert angles from degrees to radians
     double radX = angleX * M_PI / 180.0;
     double radY = angleY * M_PI / 180.0;
     double radZ = angleZ * M_PI / 180.0;
 
     // Rotation matrices
     double cosX = cos(radX), sinX = sin(radX);
     double cosY = cos(radY), sinY = sin(radY);
     double cosZ = cos(radZ), sinZ = sin(radZ);
 
     for (auto& vertex : mesh.vertices) {
         // Rotate around X-axis
         double y1 = cosX * vertex.y - sinX * vertex.z;
         double z1 = sinX * vertex.y + cosX * vertex.z;
         vertex.y = y1;
         vertex.z = z1;
 
         // Rotate around Y-axis
         double x2 = cosY * vertex.x + sinY * vertex.z;
         double z2 = -sinY * vertex.x + cosY * vertex.z;
         vertex.x = x2;
         vertex.z = z2;
 
         // Rotate around Z-axis
         double x3 = cosZ * vertex.x - sinZ * vertex.y;
         double y3 = sinZ * vertex.x + cosZ * vertex.y;
         vertex.x = x3;
         vertex.y = y3;
     }
 }
 