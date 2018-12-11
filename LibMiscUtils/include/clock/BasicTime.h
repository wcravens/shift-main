#pragma once

#include "../MiscUtils_EXPORTS.h"

#include <time.h>

namespace shift {
namespace clock {

    struct MISCUTILS_EXPORTS BasicTime {
        time_t sec;
        unsigned int usec;

        BasicTime();

        bool operator==(const BasicTime& bt);
        bool operator==(const BasicTime& bt) const;

        bool operator<(const BasicTime& bt);
        bool operator<(const BasicTime& bt) const;

        bool operator<=(const BasicTime& bt);
        bool operator<=(const BasicTime& bt) const;

        bool operator>(const BasicTime& bt);
        bool operator>(const BasicTime& bt) const;

        bool operator>=(const BasicTime& bt);
        bool operator>=(const BasicTime& bt) const;
    };

} // clock
} // shift
