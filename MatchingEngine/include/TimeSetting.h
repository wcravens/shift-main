#pragma once

#include <chrono>
#include <string>

#include <quickfix/FieldTypes.h>

#include <boost/date_time/posix_time/posix_time.hpp>

class TimeSetting {

public:
    static auto getInstance() -> TimeSetting&;
    static auto getUTCPTime(const boost::posix_time::ptime& localPtime) -> boost::posix_time::ptime;

    void initiate(const boost::posix_time::ptime& localPtime, int speed = false);
    void setStartTime();
    auto pastMilli(bool simTime = false) const -> long;
    auto pastMilli(const FIX::UtcTimeStamp& utc, bool simTime = false) const -> long;
    auto simulationTimestamp() -> FIX::UtcTimeStamp;

private:
    boost::posix_time::ptime m_utcDateTime;
    std::time_t m_hhmmss;
    int m_speed;
    std::chrono::high_resolution_clock::time_point m_startTimePoint; // real time (not simulation time)
};
