#include "MeshConverter.h"
#include <CGAL/IO/OBJ.h>   // For reading OBJ files.
#include <CGAL/IO/OFF.h>   // For writing OFF files.
#include <fstream>
#include <iostream>

MeshConverter::MeshConverter() {
    // Constructor can initialize members if needed.
}

bool MeshConverter::convert(const std::string& input_filename, const std::string& output_filename) {
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