#ifndef METADATA_EXPORTER_H
#define METADATA_EXPORTER_H

#include "MeshMetadata.h"
#include <string>

class MetadataExporter {
public:
    /**
     * @brief Exports the mesh metadata as JSON to a file.
     *
     * @param filePath The path to the output metadata file (should have a .json extension).
     * @param metadata The MeshMetadata object containing the group metadata.
     * @return true if the file was successfully written, false otherwise.
     */
    static bool exportMetadata(const std::string& filePath, const MeshMetadata& metadata);
};

#endif // METADATA_EXPORTER_H
