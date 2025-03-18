/**
 * @file MetadataExporter.cpp
 * @brief Implements functionality to export mesh metadata to a JSON file.
 */

 #include "MetadataExporter.h"
 #include <fstream>
 #include "nlohmann/json.hpp"
 
 using json = nlohmann::json;
 
 /**
  * @brief Exports mesh metadata to a JSON file.
  *
  * This function serializes the given mesh metadata into JSON format and writes it to the specified file.
  *
  * @param filePath The path of the file where metadata should be exported.
  * @param metadata The MeshMetadata object containing metadata information.
  * @return True if the metadata is successfully written, false otherwise.
  */
 bool MetadataExporter::exportMetadata(const std::string& filePath, const MeshMetadata& metadata) {
     std::ofstream file(filePath);
     if (!file.is_open()) {
         return false;
     }
     json j = metadata.toJson();
     file << j.dump(4); // Pretty print with 4-space indentation
     file.close();
     return true;
 }