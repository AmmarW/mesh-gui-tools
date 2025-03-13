#include "MetadataExporter.h"
#include <fstream>

bool MetadataExporter::exportMetadata(const std::string& filePath, const MeshMetadata& metadata) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    // Write a header.
    file << "Mesh Group Metadata\n";
    file << "====================\n\n";
    
    const auto& groups = metadata.getAllMetadata();
    for (const auto& pair : groups) {
        const auto& group = pair.second;
        file << "Group: " << group.groupName << "\n";
        file << "  Boundary Condition: " << group.boundaryCondition.type << "\n";
        file << "    Parameters: ";
        for (const auto& param : group.boundaryCondition.parameters) {
            file << param << " ";
        }
        file << "\n";
        file << "  Material Properties:\n";
        file << "    Density: " << group.materialProperties.density << "\n";
        file << "    Elastic Modulus: " << group.materialProperties.elasticModulus << "\n";
        file << "    Poisson Ratio: " << group.materialProperties.poissonRatio << "\n";
        file << "  Element Tags: ";
        for (const auto& tag : group.elementTags) {
            file << tag << " ";
        }
        file << "\n";
        file << "  Assigned Face Indices: ";
        for (const auto& faceIdx : group.faceIndices) {
            file << faceIdx << " ";
        }
        file << "\n\n";
    }
    
    file.close();
    return true;
}
