#pragma once

#include "../MiscUtils_EXPORTS.h"

#include <ctime>
#include <string>

namespace shift::clock {

class MISCUTILS_EXPORTS Timestamp {
public:
    Timestamp();
    Timestamp(std::time_t sec, int usec = 0);
    Timestamp(const std::string& dateTime, const std::string& format);

    // getters
    auto getSeconds() const -> const std::time_t&;
    auto getMicroseconds() const -> int;

    // setters
    void setSeconds(const std::time_t& sec);
    void setMicroseconds(int usec);

    auto operator==(const Timestamp& t) const -> bool;
    auto operator<(const Timestamp& t) const -> bool;
    auto operator<=(const Timestamp& t) const -> bool;
    auto operator>(const Timestamp& t) const -> bool;
    auto operator>=(const Timestamp& t) const -> bool;

private:
    std::time_t m_sec;
    int m_usec;
};

} // shift::clock
