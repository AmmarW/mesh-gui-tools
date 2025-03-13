#ifndef METADATA_EXPORTER_H
#define METADATA_EXPORTER_H

#include "MeshMetadata.h"
#include <string>

class MetadataExporter {
public:
    /**
     * @brief Exports the mesh metadata to a text file.
     *
     * The file will contain group names, boundary conditions, material properties,
     * element tags, and a list of face indices assigned to each group.
     *
     * @param filePath The path to the output metadata file.
     * @param metadata The MeshMetadata object containing the group metadata.
     * @return true if the file was successfully written, false otherwise.
     */
    static bool exportMetadata(const std::string& filePath, const MeshMetadata& metadata);
};

#endif // METADATA_EXPORTER_H
