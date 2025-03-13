/**
 * @file MeshTransformer.cpp
 * @brief Implements functions for applying transformations (translation, scaling, rotation) to a Mesh.
 */

 #include "MeshTransformer.h"
 #include <cmath>
 
 // Define M_PI if not defined
 #ifndef M_PI
 #define M_PI 3.14159265358979323846
 #endif
 
 void MeshTransformer::translate(Mesh& mesh, double tx, double ty, double tz) {
     for (auto& vertex : mesh.vertices) {
         vertex.x += tx;
         vertex.y += ty;
         vertex.z += tz;
     }
 }
 
 void MeshTransformer::scale(Mesh& mesh, double sx, double sy, double sz) {
     for (auto& vertex : mesh.vertices) {
         vertex.x *= sx;
         vertex.y *= sy;
         vertex.z *= sz;
     }
 }
 
 void MeshTransformer::rotate(Mesh& mesh, double angleX, double angleY, double angleZ) {
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
 