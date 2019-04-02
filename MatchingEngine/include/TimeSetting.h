#pragma once

#include <quickfix/FieldTypes.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <chrono>
#include <string>

class TimeSetting {
private:
    std::chrono::high_resolution_clock::time_point m_startTimePoint; // real time (not simulation time)
    boost::posix_time::ptime m_utcDateTime;
    time_t m_hhmmss;
    int m_speed;

protected:
    static boost::posix_time::ptime getUTCPTime(const boost::posix_time::ptime& pt);

public:
    void initiate(std::string date, std::string stime, int speed = false);
    void setStartTime();
    long pastMilli(bool simTime = false);
    long pastMilli(const FIX::UtcTimeStamp& utc, bool simTime = false);

    FIX::UtcTimeStamp simulationTimestamp();
};

extern TimeSetting globalTimeSetting;
