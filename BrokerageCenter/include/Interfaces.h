#pragma once

#include <mutex>
#include <string>
#include <vector>

class ITargetsInfo {
    mutable std::mutex m_mtxTargetsInfo;
    std::vector<std::string> m_targetsInfo; // (Target Computer ID, # of users from this ID)

protected:
    virtual ~ITargetsInfo() = 0;

    void registerTarget(const std::string& targetID);
    void unregisterTarget(const std::string& targetID);

public:
    std::vector<std::string> getTargetList() const;
};
