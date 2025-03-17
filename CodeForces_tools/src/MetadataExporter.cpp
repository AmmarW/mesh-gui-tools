#include "MetadataExporter.h"
#include <fstream>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

bool MetadataExporter::exportMetadata(const std::string& filePath, const MeshMetadata& metadata) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    json j = metadata.toJson();
    file << j.dump(4);
    file.close();
    return true;
}
