#include "Interfaces.h"

#include <algorithm>

/* virtual */ ITargetsInfo::~ITargetsInfo() /* = 0 */ = default;

auto ITargetsInfo::getTargetList() const -> std::vector<std::string>
{
    std::lock_guard<std::mutex> guard(m_mtxTargetsInfo);
    return m_targetsInfo;
}

void ITargetsInfo::registerTarget(const std::string& targetID)
{
    std::lock_guard<std::mutex> guard(m_mtxTargetsInfo);
    if (m_targetsInfo.end() == std::find(m_targetsInfo.begin(), m_targetsInfo.end(), targetID)) {
        m_targetsInfo.push_back(targetID);
    }
}

void ITargetsInfo::unregisterTarget(const std::string& targetID)
{
    std::lock_guard<std::mutex> guard(m_mtxTargetsInfo);
    auto pos = std::find(m_targetsInfo.begin(), m_targetsInfo.end(), targetID);
    if (m_targetsInfo.end() == pos) {
        return;
    }
    // fast removal:
    std::swap(*pos, m_targetsInfo.back());
    m_targetsInfo.pop_back();
}
