#include "ObjParser.h"
#include "ObjExporter.h"
#include "Mesh.h"
#include <iostream>
#include <limits>

void computeYLimits(const Mesh& mesh, double& minY, double& maxY) {
    minY = std::numeric_limits<double>::max();
    maxY = std::numeric_limits<double>::lowest();
    for (const auto& vertex : mesh.vertices) {
        minY = std::min(minY, vertex.y);
        maxY = std::max(maxY, vertex.y);
    }
}

void scaleMesh(Mesh& mesh, double scaleFactor) {
    for (auto& vertex : mesh.vertices) {
        vertex.x *= scaleFactor;
        vertex.y *= scaleFactor;
        vertex.z *= scaleFactor;
    }
}

void translateMesh(Mesh& mesh, double offsetY) {
    for (auto& vertex : mesh.vertices) {
        vertex.y += offsetY;
    }
}

Mesh mergeMeshes(const Mesh& mesh1, const Mesh& mesh2) {
    Mesh mergedMesh = mesh1;

    int vertexOffset = mergedMesh.vertices.size();
    int texOffset = mergedMesh.texCoords.size();
    int normalOffset = mergedMesh.normals.size();

    mergedMesh.vertices.insert(mergedMesh.vertices.end(), mesh2.vertices.begin(), mesh2.vertices.end());
    mergedMesh.texCoords.insert(mergedMesh.texCoords.end(), mesh2.texCoords.begin(), mesh2.texCoords.end());
    mergedMesh.normals.insert(mergedMesh.normals.end(), mesh2.normals.begin(), mesh2.normals.end());

    for (const auto& face : mesh2.faces) {
        Face adjustedFace;
        for (const auto& elem : face.elements) {
            adjustedFace.elements.emplace_back(
                elem.vertexIndex + vertexOffset,
                (elem.texCoordIndex != -1) ? elem.texCoordIndex + texOffset : -1,
                (elem.normalIndex != -1) ? elem.normalIndex + normalOffset : -1
            );
        }
        mergedMesh.faces.push_back(adjustedFace);
    }

    return mergedMesh;
}

int main() {
    ObjParser parser;
    ObjExporter exporter;

    Mesh humanoidOriginal = parser.parseSurfaceMesh("humanoid_robot.obj");
    Mesh sphereOriginal = parser.parseSurfaceMesh("spherical_surface_smooth.obj");

    // Humanoid standing on sphere
    Mesh humanoidStanding = humanoidOriginal;
    Mesh sphereStanding = sphereOriginal;

    double humanoidMinY, humanoidMaxY, sphereMinY, sphereMaxY;
    computeYLimits(humanoidStanding, humanoidMinY, humanoidMaxY);
    computeYLimits(sphereStanding, sphereMinY, sphereMaxY);

    double translateStandingY = sphereMaxY - humanoidMinY;
    translateMesh(humanoidStanding, translateStandingY);

    Mesh mergedStanding = mergeMeshes(humanoidStanding, sphereStanding);
    exporter.exportMesh(mergedStanding, "merged_humanoid_standing.obj");

    // Humanoid peaking out from scaled sphere (submerged except head)
    Mesh humanoidPeaking = humanoidOriginal;
    Mesh spherePeaking = sphereOriginal;

    double scalingFactor = 2.0; // Adjust this factor to control sphere size
    scaleMesh(spherePeaking, scalingFactor);

    computeYLimits(spherePeaking, sphereMinY, sphereMaxY);
    computeYLimits(humanoidPeaking, humanoidMinY, humanoidMaxY);

    double translatePeakingY = sphereMaxY - 0.15 * (humanoidMaxY - humanoidMinY) - humanoidMinY;
    translateMesh(humanoidPeaking, translatePeakingY);

    Mesh mergedPeaking = mergeMeshes(humanoidPeaking, spherePeaking);
    exporter.exportMesh(mergedPeaking, "merged_humanoid_peaking.obj");

    std::cout << "Meshes successfully exported:\n";
    std::cout << " - merged_humanoid_standing.obj\n";
    std::cout << " - merged_humanoid_peaking.obj (Scaling Factor: " << scalingFactor << ")\n";

    return 0;
}
