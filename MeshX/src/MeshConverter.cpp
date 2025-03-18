/**
 * @file MeshConverter.cpp
 * @brief Implements mesh format conversion utilities.
 */

 #include "MeshConverter.h"
 #include <CGAL/IO/OBJ.h>   // For reading OBJ files.
 #include <CGAL/IO/OFF.h>   // For writing OFF files.
 #include <fstream>
 #include <iostream>
 #include <sstream>
 
 /**
  * @brief Default constructor for the MeshConverter class.
  */
 MeshConverter::MeshConverter() {
     // Constructor can initialize members if needed.
 }
 
 /**
  * @brief Converts an OBJ file to an OFF file.
  *
  * This function reads a mesh from an OBJ file and writes it to an OFF file.
  *
  * @param input_filename Path to the input OBJ file.
  * @param output_filename Path to the output OFF file.
  * @return True if conversion is successful, false otherwise.
  */
 bool MeshConverter::convertObjToOff(const std::string& input_filename, const std::string& output_filename) {
     // Open the input OBJ file.
     std::ifstream input(input_filename);
     if (!input) {
         std::cerr << "Error: Cannot open input file " << input_filename << std::endl;
         return false;
     }
 
     CGALMesh mesh;
     // Read the OBJ file into the mesh.
     if (!CGAL::IO::read_OBJ(input, mesh)) {
         std::cerr << "Error: Failed to read OBJ file " << input_filename << std::endl;
         return false;
     }
     input.close();
 
     // Open the output OFF file.
     std::ofstream output(output_filename);
     if (!output) {
         std::cerr << "Error: Cannot open output file " << output_filename << std::endl;
         return false;
     }
 
     // Write the mesh to the OFF file.
     if (!CGAL::IO::write_OFF(output, mesh)) {
         std::cerr << "Error: Failed to write OFF file " << output_filename << std::endl;
         return false;
     }
     output.close();
 
     return true;
 }
 
 /**
  * @brief Converts an OFF file to an OBJ file.
  *
  * This function reads a mesh from an OFF file and writes it to an OBJ file.
  *
  * @param offFile Path to the input OFF file.
  * @param objFile Path to the output OBJ file.
  * @return True if conversion is successful, false otherwise.
  */
 bool MeshConverter::convertOffToObj(const std::string& offFile, const std::string& objFile) {
     // Open the OFF file for reading
     std::ifstream in(offFile);
     if (!in) {
         std::cerr << "Failed to open OFF file: " << offFile << std::endl;
         return false;
     }
 
     // Read the header and check if it's a valid OFF file
     std::string header;
     in >> header;
     if (header != "OFF") {
         std::cerr << "File " << offFile << " is not a valid OFF file." << std::endl;
         return false;
     }
 
     // Read the number of vertices, faces, and edges
     int nVertices, nFaces, nEdges;
     in >> nVertices >> nFaces >> nEdges;
 
     // Read the vertices
     std::vector<std::array<double, 3>> vertices(nVertices);
     for (int i = 0; i < nVertices; i++) {
         in >> vertices[i][0] >> vertices[i][1] >> vertices[i][2];
     }
 
     // Read the faces
     std::vector<std::vector<int>> faces(nFaces);
     for (int i = 0; i < nFaces; i++) {
         int count;
         in >> count;
         faces[i].resize(count);
         for (int j = 0; j < count; j++) {
             in >> faces[i][j];
         }
     }
     in.close();
 
     // Open the OBJ file for writing
     std::ofstream out(objFile);
     if (!out) {
         std::cerr << "Failed to open OBJ file for writing: " << objFile << std::endl;
         return false;
     }
 
     // Write the vertices to the OBJ file
     for (int i = 0; i < nVertices; i++)
         out << "v " << vertices[i][0] << " " << vertices[i][1] << " " << vertices[i][2] << "\n";
 
     // Write the faces to the OBJ file
     for (int i = 0; i < nFaces; i++) {
         out << "f";
         for (int idx : faces[i])
             out << " " << (idx + 1); // OBJ indices are 1-based
         out << "\n";
     }
     out.close();
     return true;
 }
 
 /**
  * @brief Converts a mesh to a Polyhedron.
  *
  * This function converts a mesh representation to a CGAL Polyhedron for further processing.
  *
  * @param mesh The input mesh structure.
  * @param poly The output CGAL Polyhedron.
  * @return True if conversion is successful, false otherwise.
  */
 bool MeshConverter::convertMeshToPolyhedron(const Mesh &mesh, MeshBooleanOperations::Polyhedron &poly) {
     // Create a stringstream to hold the OFF format data
     std::stringstream ss;
     ss << "OFF\n";
     ss << mesh.vertices.size() << " " << mesh.faces.size() << " 0\n";
     
     // Write vertices to the stringstream
     for (const auto &v : mesh.vertices)
         ss << v.x << " " << v.y << " " << v.z << "\n";
     
     // Write faces to the stringstream
     for (const auto &face : mesh.faces) {
         ss << face.elements.size();
         for (const auto &elem : face.elements)
             ss << " " << elem.vertexIndex;
         ss << "\n";
     }
     
     // Clear the polyhedron and read from the stringstream
     poly.clear();
     if (!(ss >> poly)) {
         std::cerr << "Error converting mesh to Polyhedron" << std::endl;
         return false;
     }
     
     // Triangulate faces of the polyhedron
     CGAL::Polygon_mesh_processing::triangulate_faces(poly);
     return true;
 }
 