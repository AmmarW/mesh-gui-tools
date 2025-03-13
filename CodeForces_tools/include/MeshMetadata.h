#ifndef MESH_METADATA_H
#define MESH_METADATA_H

#include <string>
#include <vector>
#include <map>

/**
 * @struct BoundaryCondition
 * @brief Represents the boundary condition data for an element group.
 */
struct BoundaryCondition {
    std::string type;               ///< e.g., "fixed", "free", "roller"
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
    // Additional material properties can be added here.
};

/**
 * @struct GroupMetadata
 * @brief Represents the metadata associated with an element group.
 *
 * Each group (typically created in the OBJ file via the "g" keyword)
 * can have associated boundary conditions, material properties, custom tags,
 * and a list of assigned face indices (or other elements).
 */
struct GroupMetadata {
    std::string groupName;                   ///< Name of the element group
    BoundaryCondition boundaryCondition;     ///< Boundary condition for the group
    MaterialProperties materialProperties;   ///< Material properties for the group
    std::vector<std::string> elementTags;    ///< Custom element tags for identification
    std::vector<int> faceIndices;            ///< List of face indices assigned to this group
};

/**
 * @class MeshMetadata
 * @brief Manages metadata for mesh element groups.
 */
class MeshMetadata {
public:
    /// Adds metadata for a new group.
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
     * @return true if the update was successful; false if the group was not found.
     */
    bool updateGroupMetadata(const std::string& groupName, const GroupMetadata& metadata);

    /**
     * @brief Removes metadata for a specific group.
     * @param groupName The name of the element group to remove.
     * @return true if the group was removed; false if it was not found.
     */
    bool removeGroupMetadata(const std::string& groupName);

    /// Retrieves the entire metadata mapping.
    const std::map<std::string, GroupMetadata>& getAllMetadata() const;

private:
    std::map<std::string, GroupMetadata> groupMetadataMap; ///< Mapping from group names to their metadata.
};

#endif // MESH_METADATA_H
