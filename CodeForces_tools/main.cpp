/**
 * @file main.cpp
 * @brief Main application for parsing, validating, and exporting OBJ mesh files.
 *
 * This application takes an input OBJ file, validates the mesh, and exports it to an output OBJ file.
 * 
 * Usage:
 *   ./main humanoid_robot.obj output.obj
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
 * - If the number of command-line arguments is less than 3, the application prints the usage message and exits with status 1.
 * - If any exceptions are thrown during parsing, validation, or exporting, the application prints the error message and exits with status 1.
 * - If mesh validation errors are found, they are printed to the standard error output.
 * - If the mesh export fails, an error message is printed to the standard error output.
 */
#include "ObjParser.h"
#include "MeshValidator.h"
#include "ObjExporter.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " humanoid_robot.obj output.obj\n";
        return 1;
    }
    
    std::string inputFile = argv[1];
    std::string outputFile = argv[2];
    
    try {
        Mesh mesh = ObjParser::parse(inputFile);
        
        // Validate the mesh.
        std::vector<std::string> errors = MeshValidator::validate(mesh);
        if (!errors.empty()) {
            std::cerr << "Mesh validation errors found:\n";
            for (const auto& err : errors) {
                std::cerr << "  - " << err << "\n";
            }
            // Optionally, exit if errors are critical.
            // return 1;
        } else {
            std::cout << "Mesh validation successful.\n";
        }
        
        // Export the mesh.
        if (ObjExporter::exportMesh(mesh, outputFile)) {
            std::cout << "Mesh exported successfully to " << outputFile << "\n";
        } else {
            std::cerr << "Failed to export mesh to " << outputFile << "\n";
        }
    } catch (const std::exception& ex) {
        std::cerr << "An error occurred: " << ex.what() << "\n";
        return 1;
    }
    
    return 0;
}
