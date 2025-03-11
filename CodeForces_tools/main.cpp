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
 *   ./main humanoid_robot.obj output.obj metadata.txt
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return int Exit status of the program.
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
    
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " humanoid_robot.obj output.obj metadata.txt\n";
        return 1;
    }
    
    std::string inputFile = argv[1];
    std::string outputFile = argv[2];
    std::string metadataFile = argv[3];
    
    try {
        // Step 1: Parse the input OBJ file using ObjParser::parse to create a Mesh object.
        Mesh mesh = ObjParser::parse(inputFile);
        
        // Step 2: Validate the mesh initially.
        std::vector<std::string> initialErrors = MeshValidator::validate(mesh);
        if (!initialErrors.empty()) {
            std::cerr << "Initial mesh validation errors found:\n";
            for (const auto& err : initialErrors) {
                std::cerr << "  - " << err << "\n";
            }
        } else {
            std::cout << "Initial mesh validation successful.\n";
        }
        
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
        size_t numFaces = mesh.faces.size();
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
        
        // Step 4: Revalidate the mesh after grouping.
        std::vector<std::string> postGroupingErrors = MeshValidator::validate(mesh);
        std::vector<std::string> newErrors;
        for (const auto& err : postGroupingErrors) {
            if (std::find(initialErrors.begin(), initialErrors.end(), err) == initialErrors.end()) {
                newErrors.push_back(err);
            }
        }
        if (!newErrors.empty()) {
            std::cerr << "New mesh validation errors after grouping:\n";
            for (const auto& err : newErrors) {
                std::cerr << "  - " << err << "\n";
            }
        } else {
            std::cout << "No new mesh validation errors after grouping.\n";
        }
        
        // Step 5: Export the mesh.
        if (ObjExporter::exportMesh(mesh, outputFile)) {
            std::cout << "Mesh exported successfully to " << outputFile << "\n";
        } else {
            std::cerr << "Failed to export mesh to " << outputFile << "\n";
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
