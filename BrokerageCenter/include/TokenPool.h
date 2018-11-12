#pragma once

#include "shift/miscutils/crossguid/Guid.h"

#include <set>

template<typename Uuid>
class TokenPool
{
//    using Uuid = shift::crossguid::Guid;
public:
    TokenPool(const int &size);

    Uuid take();
    void use(const Uuid &t);
    void recycle(const Uuid& t);

private:
    void move(std::set<Uuid>* from, std::set<Uuid>* to, const Uuid &val);

    std::set<Uuid> m_unusedTokens;
    std::set<Uuid> m_takenTokens;
    std::set<Uuid> m_usingTokens;
};
