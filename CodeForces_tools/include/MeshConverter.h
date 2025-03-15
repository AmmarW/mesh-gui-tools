#ifndef MESHCONVERTER_H
#define MESHCONVERTER_H

#include <string>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Surface_mesh.h>

// Define CGAL kernel and mesh types.
typedef CGAL::Simple_cartesian<double>           Kernel;
typedef Kernel::Point_3                          Point;
typedef CGAL::Surface_mesh<Point>                CGALMesh; // Renamed to avoid conflict

class MeshConverter {
public:
    // Constructor.
    MeshConverter();

    // Converts an OBJ file to an OFF file.
    // Returns true on success, false otherwise.
    bool convert(const std::string& input_filename, const std::string& output_filename);
};

#endif // MESHCONVERTER_H