/**
 * @file main.cpp
 * @brief Main program for parsing, validating, grouping, and exporting 3D mesh data from OBJ files.
 *
 * This program performs the following steps:
 * 1. Parses an input OBJ file to create a Mesh object.
 * 2. Validates the mesh initially.
 * 3. Groups mesh elements and assigns metadata.
 * 4. Revalidates the mesh after grouping.
 * 5. Exports the mesh to an output OBJ file.
 * 6. Exports the metadata to a separate text file.
 *
 * Usage:
 *   ./main humanoid_robot.obj output.obj metadata.txt [surface|volume|both]
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return int Exit status of the application.
 * 
 * The application performs the following steps:
 * 1. Parses the input OBJ file to create a Mesh object.
 * 2. Validates the Mesh object using MeshValidator.
 * 3. If validation is successful, exports the Mesh object to the output OBJ file using ObjExporter.
 * 
 * Error handling:
 * - If the number of command-line arguments is less than 5, the application prints the usage message and exits with status 1.
 * - If any exceptions are thrown during parsing, validation, or exporting, the application prints the error message and exits with status 1.
 * - If mesh validation errors are found, they are printed to the standard error output.
 * - If the mesh export fails, an error message is printed to the standard error output.
 */

 #include "ObjParser.h"
 #include "MeshValidator.h"
 #include "ObjExporter.h"
 #include "MeshMetadata.h"
 #include "MetadataExporter.h"  // New metadata exporter
 #include <iostream>
 #include <algorithm>
 #include <chrono>
 
 int main(int argc, char* argv[]) {
     // Measure the start time for runtime calculation
     auto start = std::chrono::high_resolution_clock::now();
     
     if (argc < 5) {
         std::cerr << "Usage: " << argv[0] << " humanoid_robot.obj output.obj metadata.txt [surface|volume|both]\n";
         return 1;
     }
     
     std::string inputFile = argv[1];
     std::string outputFile = argv[2];
     std::string metadataFile = argv[3];
     std::string meshType = argv[4];
     
     try {
         // Parse the Mesh
         ObjParser parser;
         Mesh surfaceMesh;
         Mesh volumeMesh;
         
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
         
         // Validate the surface mesh initially.
         std::vector<std::string> initialErrors = MeshValidator::validate(surfaceMesh);
         if (!initialErrors.empty()) {
             std::cerr << "Initial surface mesh validation errors found:\n";
             for (const auto& err : initialErrors) {
                 std::cerr << "  - " << err << "\n";
             }
         } else {
             std::cout << "Initial surface mesh validation successful.\n";
         }

         
         // TODO: Implement volume mesh initial validation.

         
         // Step 3: Grouping Phase - assign element groups and metadata.
         // In this phase, we create groups of mesh elements and assign metadata to them.
         // Each group is defined by its name, boundary conditions, material properties, and tags.
         // We then assign mesh faces to these groups based on certain criteria.
         MeshMetadata meshMetadata;
         
         // Create two groups with example metadata.
         GroupMetadata group1;
         group1.groupName = "Group1";
         group1.boundaryCondition = {"fixed", {}};              // Example: fixed boundary condition
         group1.materialProperties = {7850.0, 210e9, 0.3};        // Example: steel properties
         group1.elementTags = {"load-bearing", "critical"};
         
         GroupMetadata group2;
         group2.groupName = "Group2";
         group2.boundaryCondition = {"roller", {}};              // Example: roller boundary condition
         group2.materialProperties = {2700.0, 70e9, 0.33};         // Example: aluminum properties
         group2.elementTags = {"non-critical"};
         
         // Add the groups to the metadata manager.
         meshMetadata.addGroupMetadata(group1);
         meshMetadata.addGroupMetadata(group2);
         
         // Simulate grouping assignment:
         size_t numFaces = surfaceMesh.faces.size();
         for (size_t i = 0; i < numFaces; ++i) {
             // For demonstration, assign first half of the faces to Group1 and the rest to Group2.
             if (i < numFaces / 2) {
                 GroupMetadata* meta = meshMetadata.getGroupMetadata("Group1");
                 if (meta) {
                     meta->faceIndices.push_back(static_cast<int>(i));
                 }
             } else {
                 GroupMetadata* meta = meshMetadata.getGroupMetadata("Group2");
                 if (meta) {
                     meta->faceIndices.push_back(static_cast<int>(i));
                 }
             }
         }
         
         // Step 4: Revalidate the surface mesh after grouping.
         std::vector<std::string> postGroupingErrors = MeshValidator::validate(surfaceMesh);
         std::vector<std::string> newErrors;
         for (const auto& err : postGroupingErrors) {
             if (std::find(initialErrors.begin(), initialErrors.end(), err) == initialErrors.end()) {
                 newErrors.push_back(err);
             }
         }
         if (!newErrors.empty()) {
             std::cerr << "New surface mesh validation errors after grouping:\n";
             for (const auto& err : newErrors) {
                 std::cerr << "  - " << err << "\n";
             }
         } else {
             std::cout << "No new surface mesh validation errors after grouping.\n";
         }

        // TODO: Implement volume mesh grouping and validation.
         
         // Step 5: Export the surface mesh.
         if (meshType == "surface" || meshType == "both") {
            std::string surfaceOutputFile = "surface_" + outputFile;
             if (ObjExporter::exportMesh(surfaceMesh, surfaceOutputFile)) {
                 std::cout << "Surface mesh exported successfully to " << surfaceOutputFile  << "\n";
             } else {
                 std::cerr << "Failed to export surface mesh to " << surfaceOutputFile  <<"\n";
             }
         }
 
         // Step 5: Export the volume mesh.
         if (meshType == "volume" || meshType == "both") {
             std::string volumeOutputFile = "volume_" + outputFile;
             if (ObjExporter::exportMesh(volumeMesh, volumeOutputFile)) {
                 std::cout << "Volume mesh exported successfully to " << volumeOutputFile << "\n";
             } else {
                 std::cerr << "Failed to export volume mesh to " << volumeOutputFile << "\n";
             }
         }
         
         // Step 6: Export the metadata to a separate text file.
         if (MetadataExporter::exportMetadata(metadataFile, meshMetadata)) {
             std::cout << "Metadata exported successfully to " << metadataFile << "\n";
         } else {
             std::cerr << "Failed to export metadata to " << metadataFile << "\n";
         }
     } catch (const std::exception& ex) {
         std::cerr << "An error occurred: " << ex.what() << "\n";
         return 1;
     }
 
     // Measure and display runtime
     auto end = std::chrono::high_resolution_clock::now();
     std::chrono::duration<double> diff = end - start;
     std::cout << "Runtime: " << diff.count() << " seconds\n";
 
     return 0;
 }