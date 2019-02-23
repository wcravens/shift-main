#pragma once

#include <quickfix/FieldTypes.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <chrono>
#include <string>

class timesetting {
private:
    std::chrono::high_resolution_clock::time_point m_start_time_point; // real time (not simulation time)
    boost::posix_time::ptime m_utc_date_time;
    time_t m_hhmmss;
    int m_speed;

public:
    static boost::posix_time::ptime getUTCPTime(const boost::posix_time::ptime& pt);

    void initiate(std::string date, std::string stime, int speed = false);
    void set_start_time();
    long past_milli(bool simulation_time = false);
    long past_milli(const FIX::UtcTimeStamp& utc, bool simulation_time = false);

    FIX::UtcTimeStamp simulationTimestamp();
};

extern timesetting timepara;
