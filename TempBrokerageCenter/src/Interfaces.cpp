#include "Interfaces.h"

void ITargetsInfo::increaseTargetRefCount(const std::string& targetID)
{
    std::lock_guard<std::mutex> guard(m_mtxTargetsInfo);
    m_targetsInfo[targetID]++; // add new and/or increase ref count
}

void ITargetsInfo::decreaseTargetRefCount(const std::string& targetID)
{
    std::lock_guard<std::mutex> guard(m_mtxTargetsInfo);

    auto pos = m_targetsInfo.find(targetID);
    if (m_targetsInfo.end() == pos)
        return;

    if (--pos->second <= 0)
        m_targetsInfo.erase(pos);
}

std::vector<std::string> ITargetsInfo::getTargetList() const
{
    std::lock_guard<std::mutex> guard(m_mtxTargetsInfo);
    if (m_targetsInfo.empty())
        return {};

    std::vector<std::string> targetList;
    for (auto& kv : m_targetsInfo)
        targetList.push_back(kv.first);

    return targetList;
}
