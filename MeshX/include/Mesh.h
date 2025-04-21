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
  * Each face element references a vertex, an optional texture coordinate, and an optional normal.
  */
 struct FaceElement {
     int vertexIndex;   ///< Index into the vertices vector
     int texCoordIndex; ///< Index into the texture coordinates vector (-1 if not present)
     int normalIndex;   ///< Index into the normals vector (-1 if not present)
 
     /**
      * @brief Constructs a FaceElement with specified indices.
      * @param v Index of the vertex.
      * @param t Index of the texture coordinate (-1 if not present).
      * @param n Index of the normal (-1 if not present).
      */
     FaceElement(int v, int t = -1, int n = -1)
         : vertexIndex(v), texCoordIndex(t), normalIndex(n) {}
 };
 
 /**
  * @struct Face
  * @brief Represents a face in the mesh, which is a collection of face elements.
  */
 struct Face {
     std::vector<FaceElement> elements; ///< Collection of face elements defining the face.
 };
 
 /**
  * @class Mesh
  * @brief Represents the entire mesh data.
  *
  * The Mesh class holds the vertices, faces, normals, and texture coordinates of the mesh.
  */
 class Mesh {
 public:
     std::vector<Vertex> vertices; ///< List of vertices in the mesh.
     std::vector<Face> faces; ///< List of faces in the mesh.
     std::vector<Vertex> normals; ///< Optional normals for shading calculations.
     std::vector<std::array<double, 2>> texCoords; ///< Optional texture coordinates.
 
     /**
      * @typedef Tetrahedron
      * @brief Represents a tetrahedral element in a volumetric mesh.
      *
      * A Tetrahedron is represented as a tuple containing four vertex indices.
      */
     using Tetrahedron = std::tuple<int, int, int, int>;
     std::vector<Tetrahedron> tetrahedrons; ///< List of tetrahedrons for volumetric meshes.
 };
 
 #endif // MESH_H
 