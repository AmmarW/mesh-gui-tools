#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/IO/OBJ.h>  // Use this for reading/writing OBJ files

#include <iostream>

namespace PMP = CGAL::Polygon_mesh_processing;

// Define kernel and mesh type
typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef CGAL::Surface_mesh<Kernel::Point_3> Mesh;

// Function to ensure the mesh is triangular
void ensure_triangulated(Mesh& mesh) {
    if (!CGAL::is_triangle_mesh(mesh)) {  // Use CGAL::is_triangle_mesh() instead of PMP::is_triangle_mesh()
        std::cout << "Triangulating the input mesh...\n";
        PMP::triangulate_faces(mesh);
    }
}

// Function to perform Boolean operations and save the result
void perform_boolean_operation(const std::string& file1, const std::string& file2, 
                               const std::string& output_file, const std::string& operation) {
    Mesh mesh1, mesh2, result;

    // Load the input OBJ files
    if (!CGAL::IO::read_OBJ(file1, mesh1) || !CGAL::IO::read_OBJ(file2, mesh2)) {
        std::cerr << "Error: Could not load one of the input meshes.\n";
        return;
    }

    // Ensure both meshes are triangulated
    ensure_triangulated(mesh1);
    ensure_triangulated(mesh2);

    // Perform Boolean operation
    if (operation == "union") {
        PMP::corefine_and_compute_union(mesh1, mesh2, result);
    } else if (operation == "difference") {
        PMP::corefine_and_compute_difference(mesh1, mesh2, result);
    } else if (operation == "intersection") {
        PMP::corefine_and_compute_intersection(mesh1, mesh2, result);
    } else {
        std::cerr << "Error: Invalid operation. Use 'union', 'difference', or 'intersection'.\n";
        return;
    }

    // Save the output mesh using CGAL::IO::write_OBJ()
    if (!CGAL::IO::write_OBJ(output_file, result)) {
        std::cerr << "Error: Failed to save the output mesh.\n";
        return;
    }

    std::cout << "Boolean operation '" << operation << "' completed successfully.\n";
}

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <file1.obj> <file2.obj> <output.obj> <operation>\n";
        std::cerr << "Operations: union, difference, intersection\n";
        return 1;
    }

    std::string file1 = argv[1];
    std::string file2 = argv[2];
    std::string output_file = argv[3];
    std::string operation = argv[4];

    perform_boolean_operation(file1, file2, output_file, operation);
    
    return 0;
}
