#ifndef ADAPTIVE_MESH_GENERATOR_H
#define ADAPTIVE_MESH_GENERATOR_H

#include <string>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Mesh_polyhedron_3.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/boost/graph/copy_face_graph.h>

namespace params = CGAL::parameters;

class AdaptiveMeshGenerator {
public:
    // Define types used in the meshing process.
    typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
    typedef Kernel::Point_3 Point;
    typedef CGAL::Surface_mesh<Point> SurfaceMesh;
    typedef CGAL::Mesh_polyhedron_3<Kernel>::type Polyhedron;
    
    AdaptiveMeshGenerator();

    // Generates the volume mesh by computing the Boolean difference (cube \ inner).
    // innerMeshFile: Path to the inner mesh OFF file.
    // cubeSize: Half-size of the cube (cube extends from -cubeSize to cubeSize).
    // outputMeshFile: Filename for the output mesh (dumped using CGAL::dump_c3t3).
    // Returns true if the mesh is generated successfully.
    bool generateVolumeMesh(const std::string &innerMeshFile,
                            int cubeSize,
                            const std::string &outputMeshFile = "adaptive_mesh.off");
};

#endif // ADAPTIVE_MESH_GENERATOR_H
