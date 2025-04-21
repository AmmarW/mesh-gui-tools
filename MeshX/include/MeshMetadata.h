/**
 * @file MeshMetadata.h
 * @brief Defines structures and classes for managing metadata of mesh element groups.
 *
 * This file provides definitions for metadata structures such as boundary conditions,
 * material properties, and spatial data associated with mesh element groups.
 */

 #ifndef MESH_METADATA_H
 #define MESH_METADATA_H
 
 #include <string>
 #include <vector>
 #include <map>
 #include <array>
 #include "nlohmann/json.hpp"
 
 using json = nlohmann::json;
 
 /**
  * @struct BoundaryCondition
  * @brief Represents the boundary condition data for an element group.
  */
 struct BoundaryCondition {
     std::string type;               ///< e.g., "fixed", "roller", "free"
     std::vector<double> parameters; ///< Additional parameters for the condition
 };
 
 /**
  * @struct MaterialProperties
  * @brief Represents simulation-specific material properties.
  */
 struct MaterialProperties {
     double density;         ///< Material density
     double elasticModulus;  ///< Young's modulus
     double poissonRatio;    ///< Poisson's ratio
 };
 
 /**
  * @struct FaceSpatialData
  * @brief Represents spatial data for a face.
  */
 struct FaceSpatialData {
     int faceIndex;                           ///< Index of the face.
     std::array<double, 3> centroid;          ///< Centroid coordinates (x, y, z).
     std::vector<std::array<double, 3>> vertices; ///< Optional: Full vertex coordinates for the face.
 };
 
 /**
  * @struct GroupMetadata
  * @brief Represents the metadata associated with an element group.
  */
 struct GroupMetadata {
     std::string groupName;                   ///< Name of the element group.
     BoundaryCondition boundaryCondition;     ///< Boundary condition for the group.
     MaterialProperties materialProperties;   ///< Material properties for the group.
     std::vector<std::string> elementTags;    ///< Custom element tags.
     std::vector<int> faceIndices;            ///< List of face indices assigned to this group.
     std::vector<FaceSpatialData> spatialData;///< Spatial data for each face.
 };
 
 /**
  * @class MeshMetadata
  * @brief Manages metadata for mesh element groups.
  */
 class MeshMetadata {
 public:
     /**
      * @brief Adds metadata for a new group.
      * @param metadata The metadata to be added.
      */
     void addGroupMetadata(const GroupMetadata& metadata);
 
     /**
      * @brief Retrieves metadata for a specific group.
      * @param groupName The name of the element group.
      * @return Pointer to the metadata if found; nullptr otherwise.
      */
     GroupMetadata* getGroupMetadata(const std::string& groupName);
 
     /**
      * @brief Updates metadata for an existing group.
      * @param groupName The name of the element group.
      * @param metadata The new metadata to associate with the group.
      * @return true if the update was successful; false otherwise.
      */
     bool updateGroupMetadata(const std::string& groupName, const GroupMetadata& metadata);
 
     /**
      * @brief Removes metadata for a specific group.
      * @param groupName The name of the element group to remove.
      * @return true if the group was removed; false otherwise.
      */
     bool removeGroupMetadata(const std::string& groupName);
 
     /**
      * @brief Retrieves the entire metadata mapping.
      * @return A constant reference to the metadata map.
      */
     const std::map<std::string, GroupMetadata>& getAllMetadata() const;
 
     /**
      * @brief Converts the metadata to a JSON object.
      * @return A JSON representation of the metadata.
      */
     json toJson() const;
 
 private:
     std::map<std::string, GroupMetadata> groupMetadataMap; ///< Mapping from group names to their metadata.
 };
 
 #endif // MESH_METADATA_H
 