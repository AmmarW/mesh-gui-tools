#include "MeshMetadata.h"

void MeshMetadata::addGroupMetadata(const GroupMetadata& metadata) {
    groupMetadataMap[metadata.groupName] = metadata;
}

GroupMetadata* MeshMetadata::getGroupMetadata(const std::string& groupName) {
    auto it = groupMetadataMap.find(groupName);
    if (it != groupMetadataMap.end()) {
        return &it->second;
    }
    return nullptr;
}

bool MeshMetadata::updateGroupMetadata(const std::string& groupName, const GroupMetadata& metadata) {
    auto it = groupMetadataMap.find(groupName);
    if (it != groupMetadataMap.end()) {
        it->second = metadata;
        return true;
    }
    return false;
}

bool MeshMetadata::removeGroupMetadata(const std::string& groupName) {
    return groupMetadataMap.erase(groupName) > 0;
}

const std::map<std::string, GroupMetadata>& MeshMetadata::getAllMetadata() const {
    return groupMetadataMap;
}

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
