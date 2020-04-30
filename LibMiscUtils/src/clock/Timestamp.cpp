#include "clock/Timestamp.h"

#include <iomanip>
#include <sstream>

namespace shift::clock {

Timestamp::Timestamp()
    : Timestamp { time(nullptr), 0 }
{
}

Timestamp::Timestamp(std::time_t sec, int usec /* = 0 */)
    : m_sec { sec }
    , m_usec { usec }
{
}

Timestamp::Timestamp(const std::string& dateTime, const std::string& format)
    : m_usec { 0 }
{
    struct std::tm c_tm = { 0 };
    c_tm.tm_isdst = -1; // required for correct initialization

    std::istringstream ss(dateTime);
    ss >> std::get_time(&c_tm, format.c_str());

    m_sec = mktime(&c_tm);
}

auto Timestamp::getSeconds() const -> const std::time_t&
{
    return m_sec;
}

auto Timestamp::getMicroseconds() const -> int
{
    return m_usec;
}

void Timestamp::setSeconds(const std::time_t& sec)
{
    m_sec = sec;
}

void Timestamp::setMicroseconds(int usec)
{
    m_usec = usec;
}

auto Timestamp::operator==(const Timestamp& t) const -> bool
{
    return (this->m_sec == t.m_sec) && (this->m_usec == t.m_usec);
}

auto Timestamp::operator<(const Timestamp& t) const -> bool
{
    if (this->m_sec < t.m_sec) {
        return true;
    }

    if (this->m_sec == t.m_sec) {
        if (this->m_usec < t.m_usec) {
            return true;
        }
    }

    return false;
}

auto Timestamp::operator<=(const Timestamp& t) const -> bool
{
    return !(*this > t);
}

auto Timestamp::operator>(const Timestamp& t) const -> bool
{
    if (this->m_sec > t.m_sec) {
        return true;
    }

    if (this->m_sec == t.m_sec) {
        if (this->m_usec > t.m_usec) {
            return true;
        }
    }

    return false;
}

auto Timestamp::operator>=(const Timestamp& t) const -> bool
{
    return !(*this < t);
}

} // shift::clock
