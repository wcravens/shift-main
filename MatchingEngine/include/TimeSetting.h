#pragma once

#include <quickfix/FieldTypes.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <chrono>
#include <string>

class TimeSetting {

public:
    static boost::posix_time::ptime getUTCPTime(const boost::posix_time::ptime& pt);

    static TimeSetting& getGlobalTimeSetting();

    void initiate(std::string date, std::string stime, int speed = false);
    void setStartTime();
    long pastMilli(bool simTime = false);
    long pastMilli(const FIX::UtcTimeStamp& utc, bool simTime = false);
    FIX::UtcTimeStamp simulationTimestamp();

private:
    boost::posix_time::ptime m_utcDateTime;
    std::time_t m_hhmmss;
    int m_speed;
    std::chrono::high_resolution_clock::time_point m_startTimePoint; // real time (not simulation time)
};
