#include "clock/Timestamp.h"

#include <iomanip>
#include <sstream>

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

shift::clock::Timestamp::Timestamp(const std::string& dateTime, const std::string& format)
    : m_usec{ 0 }
{
    struct std::tm tm = { 0 };
    tm.tm_isdst = -1; // required for correct initialization

    std::istringstream ss(dateTime);
    ss >> std::get_time(&tm, format.c_str());

    m_sec = mktime(&tm);
}

const std::time_t& shift::clock::Timestamp::getSeconds() const
{
    return m_sec;
}

int shift::clock::Timestamp::getMicroseconds() const
{
    return m_usec;
}

void shift::clock::Timestamp::setSeconds(const std::time_t& sec)
{
    m_sec = sec;
}

void shift::clock::Timestamp::setMicroseconds(int usec)
{
    m_usec = usec;
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
