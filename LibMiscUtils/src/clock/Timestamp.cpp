#include "clock/Timestamp.h"

shift::clock::Timestamp::Timestamp()
    : m_sec{ time(nullptr) }
    , m_usec{ 0 }
{
}

shift::clock::Timestamp::Timestamp(const std::time_t& sec, int usec)
    : m_sec{ sec }
    , m_usec{ usec }
{
}

bool shift::clock::Timestamp::operator==(const Timestamp& t)
{
    return (this->m_sec == t.m_sec) && (this->m_usec == t.m_usec);
}

bool shift::clock::Timestamp::operator==(const Timestamp& t) const
{
    return (this->m_sec == t.m_sec) && (this->m_usec == t.m_usec);
}

bool shift::clock::Timestamp::operator<(const Timestamp& t)
{
    if (this->m_sec < t.m_sec) {
        return true;
    } else if (this->m_sec == t.m_sec) {
        if (this->m_usec < t.m_usec) {
            return true;
        }
    }

    return false;
}

bool shift::clock::Timestamp::operator<(const Timestamp& t) const
{
    if (this->m_sec < t.m_sec) {
        return true;
    } else if (this->m_sec == t.m_sec) {
        if (this->m_usec < t.m_usec) {
            return true;
        }
    }

    return false;
}

bool shift::clock::Timestamp::operator<=(const Timestamp& t)
{
    return !(*this > t);
}

bool shift::clock::Timestamp::operator<=(const Timestamp& t) const
{
    return !(*this > t);
}

bool shift::clock::Timestamp::operator>(const Timestamp& t)
{
    if (this->m_sec > t.m_sec) {
        return true;
    } else if (this->m_sec == t.m_sec) {
        if (this->m_usec > t.m_usec) {
            return true;
        }
    }

    return false;
}

bool shift::clock::Timestamp::operator>(const Timestamp& t) const
{
    if (this->m_sec > t.m_sec) {
        return true;
    } else if (this->m_sec == t.m_sec) {
        if (this->m_usec > t.m_usec) {
            return true;
        }
    }

    return false;
}

bool shift::clock::Timestamp::operator>=(const Timestamp& t)
{
    return !(*this < t);
}

bool shift::clock::Timestamp::operator>=(const Timestamp& t) const
{
    return !(*this < t);
}