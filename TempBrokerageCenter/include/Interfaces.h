#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class ITargetsInfo {
    mutable std::mutex m_mtxTargetsInfo;
    std::unordered_map<std::string, int> m_targetsInfo; // (Target Computer ID, # of users from this ID)

protected:
    void increaseTargetRefCount(const std::string& targetID);
    void decreaseTargetRefCount(const std::string& targetID);
    std::vector<std::string> getTargetList() const;
}