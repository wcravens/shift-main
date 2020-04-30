#pragma once

#include <set>

#include <shift/miscutils/crossguid/Guid.h>

template <typename Uuid>
class TokenPool {
public:
    TokenPool(int size);

    auto take() -> Uuid;
    void use(const Uuid& t);
    void recycle(const Uuid& t);

private:
    void move(std::set<Uuid>* from, std::set<Uuid>* to, const Uuid& val);

    std::set<Uuid> m_unusedTokens;
    std::set<Uuid> m_takenTokens;
    std::set<Uuid> m_usingTokens;
};

template <typename Uuid>
TokenPool<Uuid>::TokenPool(int size)
{
    for (int i = 0; i < size; ++i) {
        m_unusedTokens.insert(shift::crossguid::newGuid());
    }
}

template <typename Uuid>
auto TokenPool<Uuid>::take() -> Uuid
{
    Uuid t = *m_unusedTokens.begin();
    move(&m_unusedTokens, &m_takenTokens, t);
    return t;
}

template <typename Uuid>
void TokenPool<Uuid>::use(const Uuid& t)
{
    move(&m_takenTokens, &m_usingTokens, t);
}

template <typename Uuid>
void TokenPool<Uuid>::recycle(const Uuid& t)
{
    move(&m_usingTokens, &m_unusedTokens, t);
}

template <typename Uuid>
void TokenPool<Uuid>::move(std::set<Uuid>* from, std::set<Uuid>* to, const Uuid& val)
{
    auto i = from->find(val);
    Uuid t = *i;
    from->erase(i);
    to->insert(t);
}
