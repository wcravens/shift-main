#pragma once

#include <quickfix/FieldTypes.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <chrono>
#include <string>

class timesetting {
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
    long pastMilli(bool simulation_time = false);
    long pastMilli(const FIX::UtcTimeStamp& utc, bool simulation_time = false);

    FIX::UtcTimeStamp simulationTimestamp();
};

extern timesetting timepara;
