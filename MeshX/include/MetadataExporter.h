/**
 * @file MetadataExporter.h
 * @brief Defines the MetadataExporter class for exporting mesh metadata to a JSON file.
 *
 * This file provides the MetadataExporter class, which includes functionality to export
 * MeshMetadata objects to a JSON file format.
 */

 #ifndef METADATA_EXPORTER_H
 #define METADATA_EXPORTER_H
 
 #include "MeshMetadata.h"
 #include <string>
 
 /**
  * @class MetadataExporter
  * @brief Provides functionality to export mesh metadata as JSON.
  */
 class MetadataExporter {
 public:
     /**
      * @brief Exports the mesh metadata as JSON to a file.
      *
      * This function writes the provided MeshMetadata object to a specified file
      * in JSON format.
      *
      * @param filePath The path to the output metadata file (should have a .json extension).
      * @param metadata The MeshMetadata object containing the group metadata.
      * @return True if the file was successfully written, false otherwise.
      */
     static bool exportMetadata(const std::string& filePath, const MeshMetadata& metadata);
 };
 
 #endif // METADATA_EXPORTER_H