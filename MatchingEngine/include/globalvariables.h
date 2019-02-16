#pragma once

#include <quickfix/FieldTypes.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <chrono>
#include <string>

class timesetting {
private:
    std::chrono::high_resolution_clock::time_point start_time_point; // real time (not simulation time)
    boost::posix_time::ptime utc_date_time;
    long hhmmss;
    double speed;

public:
    static boost::posix_time::ptime getUTCPTime(const boost::posix_time::ptime& pt);

    void initiate(std::string date, std::string stime, double _speed);
    void set_start_time();
    double past_milli();
    double past_milli(const FIX::UtcTimeStamp& utc);

    FIX::UtcTimeStamp milli2utc(double milli);
    FIX::UtcTimeStamp utc_now();
};

extern timesetting timepara;
