/**
 * @file MeshMetadata.cpp
 * @brief Handles metadata management for mesh groups.
 */

 #include "MeshMetadata.h"

 /**
  * @brief Adds metadata for a mesh group.
  *
  * @param metadata The metadata associated with the mesh group.
  */
 void MeshMetadata::addGroupMetadata(const GroupMetadata& metadata) {
     groupMetadataMap[metadata.groupName] = metadata;
 }
 
 /**
  * @brief Retrieves metadata for a specific mesh group.
  *
  * @param groupName The name of the group whose metadata is requested.
  * @return Pointer to the metadata if found, otherwise nullptr.
  */
 GroupMetadata* MeshMetadata::getGroupMetadata(const std::string& groupName) {
     auto it = groupMetadataMap.find(groupName);
     if (it != groupMetadataMap.end()) {
         return &it->second;
     }
     return nullptr;
 }
 
 /**
  * @brief Updates the metadata for a given mesh group.
  *
  * @param groupName The name of the group to update.
  * @param metadata The updated metadata.
  * @return True if the update is successful, false if the group is not found.
  */
 bool MeshMetadata::updateGroupMetadata(const std::string& groupName, const GroupMetadata& metadata) {
     auto it = groupMetadataMap.find(groupName);
     if (it != groupMetadataMap.end()) {
         it->second = metadata;
         return true;
     }
     return false;
 }
 
 /**
  * @brief Removes metadata for a specified mesh group.
  *
  * @param groupName The name of the group to remove.
  * @return True if the group metadata was removed, false if the group was not found.
  */
 bool MeshMetadata::removeGroupMetadata(const std::string& groupName) {
     return groupMetadataMap.erase(groupName) > 0;
 }
 
 /**
  * @brief Retrieves all stored mesh group metadata.
  *
  * @return A constant reference to the map of all group metadata.
  */
 const std::map<std::string, GroupMetadata>& MeshMetadata::getAllMetadata() const {
     return groupMetadataMap;
 }
 
 /**
  * @brief Converts the mesh metadata to a JSON representation.
  *
  * @return A JSON object representing the mesh metadata.
  */
 json MeshMetadata::toJson() const {
     json j;
     for (const auto& pair : groupMetadataMap) {
         const GroupMetadata& group = pair.second;
         json jGroup;
         jGroup["groupName"] = group.groupName;
         jGroup["boundaryCondition"] = {
             {"type", group.boundaryCondition.type},
             {"parameters", group.boundaryCondition.parameters}
         };
         jGroup["materialProperties"] = {
             {"density", group.materialProperties.density},
             {"elasticModulus", group.materialProperties.elasticModulus},
             {"poissonRatio", group.materialProperties.poissonRatio}
         };
         jGroup["elementTags"] = group.elementTags;
         jGroup["faceIndices"] = group.faceIndices;
         jGroup["spatialData"] = json::array();
         for (const auto& fsd : group.spatialData) {
             json jFsd;
             jFsd["faceIndex"] = fsd.faceIndex;
             jFsd["centroid"] = {fsd.centroid[0], fsd.centroid[1], fsd.centroid[2]};
             jFsd["vertices"] = json::array();
             for (const auto &vert : fsd.vertices) {
                 jFsd["vertices"].push_back({vert[0], vert[1], vert[2]});
             }
             jGroup["spatialData"].push_back(jFsd);
         }
         // Use group name as the key.
         j[group.groupName] = jGroup;
     }
     return j;
 }
 