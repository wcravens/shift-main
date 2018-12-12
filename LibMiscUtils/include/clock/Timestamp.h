#pragma once

#include "../MiscUtils_EXPORTS.h"

#include <ctime>

namespace shift {
namespace clock {

    struct MISCUTILS_EXPORTS Timestamp {
        std::time_t m_sec;
        int m_usec;

        Timestamp();
        Timestamp(const std::time_t& sec, int usec);

        bool operator==(const Timestamp& t);
        bool operator==(const Timestamp& t) const;

        bool operator<(const Timestamp& t);
        bool operator<(const Timestamp& t) const;

        bool operator<=(const Timestamp& t);
        bool operator<=(const Timestamp& t) const;

        bool operator>(const Timestamp& t);
        bool operator>(const Timestamp& t) const;

        bool operator>=(const Timestamp& t);
        bool operator>=(const Timestamp& t) const;
    };

} // clock
} // shift
