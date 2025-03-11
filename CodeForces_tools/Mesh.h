/**
 * @file Mesh.h
 * @brief Defines structures and classes for representing a 3D mesh.
 *
 * This file contains the definitions for the Vertex, FaceElement, Face, and Mesh
 * classes, which are used to represent the geometry of a 3D mesh. The Mesh class
 * holds the entire mesh data, including vertices, faces, normals, and texture coordinates.
 */

 #ifndef MESH_H
 #define MESH_H
 
 #include <vector>
 #include <array>
 #include <tuple>
 
 /**
  * @struct Vertex
  * @brief Represents a 3D vertex with x, y, and z coordinates.
  */
 struct Vertex {
     double x, y, z;
 };
 
 /**
  * @struct FaceElement
  * @brief Represents an element of a face, including vertex, texture, and normal indices.
  *
  * @param vertexIndex Index into the vertices vector.
  * @param texCoordIndex Index into the texture coordinates vector (-1 if not present).
  * @param normalIndex Index into the normals vector (-1 if not present).
  */
 struct FaceElement {
     int vertexIndex;   // Index into the vertices vector
     int texCoordIndex; // Index into the texture coordinates vector (-1 if not present)
     int normalIndex;   // Index into the normals vector (-1 if not present)
 
     FaceElement(int v, int t = -1, int n = -1)
         : vertexIndex(v), texCoordIndex(t), normalIndex(n) {}
 };
 
 /**
  * @struct Face
  * @brief Represents a face in the mesh, which is a collection of face elements.
  */
 struct Face {
     std::vector<FaceElement> elements;
 };
 
 /**
  * @class Mesh
  * @brief Represents the entire mesh data.
  *
  * The Mesh class holds the vertices, faces, normals, and texture coordinates of the mesh.
  *
  * @var Mesh::vertices
  * A vector of Vertex structures representing the vertices of the mesh.
  *
  * @var Mesh::faces
  * A vector of Face structures representing the faces of the mesh.
  *
  * @var Mesh::normals
  * A vector of Vertex structures representing the normals of the mesh (optional).
  *
  * @var Mesh::texCoords
  * A vector of 2D arrays representing the texture coordinates of the mesh (optional).
  *
  * @var Mesh::tetrahedrons
  * A vector of Tetrahedron tuples representing the tetrahedrons of the volume mesh.
  */
 class Mesh {
 public:
     std::vector<Vertex> vertices;
     std::vector<Face> faces;
     std::vector<Vertex> normals;             // Optional normals
     std::vector<std::array<double, 2>> texCoords; // Optional texture coordinates
 
     // New data structure for volume mesh (tetrahedrons)
     using Tetrahedron = std::tuple<int, int, int, int>;
     std::vector<Tetrahedron> tetrahedrons;
 };
 
 #endif // MESH_H