/**
 * @file main.cpp
 * @brief Main program for parsing, validating, transforming, grouping, and exporting 3D mesh data from OBJ files.
 *
 * This program performs the following steps:
 * 1. Parses an input OBJ file to create a Mesh object.
 * 2. Validates the mesh initially.
 * 3. Applies transformations (translation, scaling, rotation) based on user input.
 * 4. Groups mesh elements and assigns metadata.
 * 5. Revalidates the mesh after grouping.
 * 6. Exports the transformed mesh to an output OBJ file.
 * 7. Exports the metadata to a separate text file.
 *
 * Transformation Parameters:
 * - `-t x y z` → Translate the mesh by x, y, and z units.
 * - `-s x y z` → Scale the mesh by factors along x, y, and z.
 * - `-r x y z` → Rotate the mesh by x, y, and z degrees.
 *
 * Usage:
 *   ./main humanoid_robot.obj output.obj metadata.txt [surface|volume|both] [-t x y z] [-s x y z] [-r x y z]
 *
 * Example:
 *   ./main humanoid_robot.obj output.obj metadata.txt both -t 1 2 3 -s 1.5 1.5 1.5 -r 45 30 60
 */

 #include "ObjParser.h"
 #include "MeshValidator.h"
 #include "ObjExporter.h"
 #include "MeshMetadata.h"
 #include "MetadataExporter.h"
 #include "MeshTransformer.h"  // Include transformation header
 #include <iostream>
 #include <algorithm>
 #include <chrono>
 
 int main(int argc, char* argv[]) {
     auto start = std::chrono::high_resolution_clock::now();
 
     if (argc < 5) {
         std::cerr << "Usage: " << argv[0] << " humanoid_robot.obj output.obj metadata.txt [surface|volume|both] [-t x y z] [-s x y z] [-r x y z]\n";
         return 1;
     }
 
     std::string inputFile = argv[1];
     std::string outputFile = argv[2];
     std::string metadataFile = argv[3];
     std::string meshType = argv[4];
 
     // Default transformation values (no transformation applied)
     double tx = 0, ty = 0, tz = 0;
     double sx = 1, sy = 1, sz = 1;
     double rx = 0, ry = 0, rz = 0;
 
     // Parse additional transformation parameters
     for (int i = 5; i < argc; ++i) {
         std::string arg = argv[i];
         if (arg == "-t" && i + 3 < argc) {  // Translation
             tx = std::stod(argv[++i]);
             ty = std::stod(argv[++i]);
             tz = std::stod(argv[++i]);
         } else if (arg == "-s" && i + 3 < argc) {  // Scaling
             sx = std::stod(argv[++i]);
             sy = std::stod(argv[++i]);
             sz = std::stod(argv[++i]);
         } else if (arg == "-r" && i + 3 < argc) {  // Rotation
             rx = std::stod(argv[++i]);
             ry = std::stod(argv[++i]);
             rz = std::stod(argv[++i]);
         }
     }
 
     try {
         ObjParser parser;
         Mesh surfaceMesh;
         Mesh volumeMesh;
 
         // Parse the Mesh
         if (meshType == "surface") {
             surfaceMesh = parser.parseSurfaceMesh(inputFile);
         } else if (meshType == "volume") {
             volumeMesh = parser.parseVolumeMesh(inputFile);
         } else if (meshType == "both") {
             surfaceMesh = parser.parseSurfaceMesh(inputFile);
             volumeMesh = parser.parseVolumeMesh(inputFile);
         } else {
             std::cerr << "Invalid mesh type. Use 'surface', 'volume', or 'both'.\n";
             return 1;
         }
 
         // Apply transformations to the surface mesh if applicable
         if (meshType == "surface" || meshType == "both") {
             std::cout << "Applying transformations to surface mesh...\n";
 
             if (tx != 0 || ty != 0 || tz != 0) {
                 MeshTransformer::translate(surfaceMesh, tx, ty, tz);
                 std::cout << "  - Translated by (" << tx << ", " << ty << ", " << tz << ")\n";
             }
             if (sx != 1 || sy != 1 || sz != 1) {
                 MeshTransformer::scale(surfaceMesh, sx, sy, sz);
                 std::cout << "  - Scaled by factors (" << sx << ", " << sy << ", " << sz << ")\n";
             }
             if (rx != 0 || ry != 0 || rz != 0) {
                 MeshTransformer::rotate(surfaceMesh, rx, ry, rz);
                 std::cout << "  - Rotated by (" << rx << "°, " << ry << "°, " << rz << "°)\n";
             }
         }
 
         // Apply transformations to the volume mesh if applicable
         if (meshType == "volume" || meshType == "both") {
             std::cout << "Applying transformations to volume mesh...\n";
 
             if (tx != 0 || ty != 0 || tz != 0) {
                 MeshTransformer::translate(volumeMesh, tx, ty, tz);
                 std::cout << "  - Translated by (" << tx << ", " << ty << ", " << tz << ")\n";
             }
             if (sx != 1 || sy != 1 || sz != 1) {
                 MeshTransformer::scale(volumeMesh, sx, sy, sz);
                 std::cout << "  - Scaled by factors (" << sx << ", " << sy << ", " << sz << ")\n";
             }
             if (rx != 0 || ry != 0 || rz != 0) {
                 MeshTransformer::rotate(volumeMesh, rx, ry, rz);
                 std::cout << "  - Rotated by (" << rx << "°, " << ry << "°, " << rz << "°)\n";
             }
         }
 
         // Export transformed mesh
         if (meshType == "surface" || meshType == "both") {
             std::string surfaceOutputFile = "surface_" + outputFile;
             if (ObjExporter::exportMesh(surfaceMesh, surfaceOutputFile)) {
                 std::cout << "Surface mesh exported successfully to " << surfaceOutputFile << "\n";
             } else {
                 std::cerr << "Failed to export surface mesh to " << surfaceOutputFile << "\n";
             }
         }
 
         if (meshType == "volume" || meshType == "both") {
             std::string volumeOutputFile = "volume_" + outputFile;
             if (ObjExporter::exportMesh(volumeMesh, volumeOutputFile)) {
                 std::cout << "Volume mesh exported successfully to " << volumeOutputFile << "\n";
             } else {
                 std::cerr << "Failed to export volume mesh to " << volumeOutputFile << "\n";
             }
         }
 
     } catch (const std::exception& ex) {
         std::cerr << "An error occurred: " << ex.what() << "\n";
         return 1;
     }
 
     auto end = std::chrono::high_resolution_clock::now();
     std::cout << "Runtime: " << std::chrono::duration<double>(end - start).count() << " seconds\n";
 
     return 0;
 }
 