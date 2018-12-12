#pragma once

#include "../MiscUtils_EXPORTS.h"

#include <ctime>
#include <string>

namespace shift {
namespace clock {

    class MISCUTILS_EXPORTS Timestamp {
    public:
        Timestamp();
        Timestamp(const std::time_t& sec, int usec = 0);
        Timestamp(const std::string& dateTime, const std::string& format);

        // Getters
        const std::time_t& getSeconds() const;
        int getMicroseconds() const;

        // Setters
        void setSeconds(const std::time_t& sec);
        void setMicroseconds(int usec);

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

    private:
        std::time_t m_sec;
        int m_usec;
    };

} // clock
} // shift
