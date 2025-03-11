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
