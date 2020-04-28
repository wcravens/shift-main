#pragma once

#include <mutex>
#include <string>
#include <vector>

class ITargetsInfo {
public:
    auto getTargetList() const -> std::vector<std::string>;

protected:
    virtual ~ITargetsInfo() = 0;

    void registerTarget(const std::string& targetID);
    void unregisterTarget(const std::string& targetID);

private:
    mutable std::mutex m_mtxTargetsInfo;
    std::vector<std::string> m_targetsInfo; // (Target Computer ID, # of users from this ID)
};
