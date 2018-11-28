#include "TokenPool.h"

template<typename Uuid>
TokenPool<Uuid>::TokenPool(const int &size)
{
    for (int i = 0; i < size; ++ i)
        m_unusedTokens.insert(shift::crossguid::newGuid());
}

template<typename Uuid>
Uuid TokenPool<Uuid>::take()
{
    Uuid t = *m_unusedTokens.begin();
    move(&m_unusedTokens, &m_takenTokens, t);
    return t;
}

template<typename Uuid>
void TokenPool<Uuid>::use(const Uuid &t)
{
    move(&m_takenTokens, &m_usingTokens, t);
}

template<typename Uuid>
void TokenPool<Uuid>::recycle(const Uuid &t)
{
    move(&m_usingTokens, &m_unusedTokens, t);
}

template<typename Uuid>
void TokenPool<Uuid>::move(std::set<Uuid> *from, std::set<Uuid> *to, const Uuid &val)
{
    auto i = from->find(val);
    Uuid t = *i;
    from->erase(i);
    to->insert(t);
}
