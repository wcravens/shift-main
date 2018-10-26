#pragma once

#include <chrono>
#include <string>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <quickfix/Application.h>

class timesetting {
private:
    boost::posix_time::ptime date_time;
    std::string Innerendtime;
    std::string Innerstarttime;
    std::string Innerstartdate;
    //time_t innerstartLong; //difficult to use because of Winterzeit and summertime problems
    long hhmmss;
    std::chrono::high_resolution_clock::time_point start_time_point;
    double speed;

public:
    static std::string now_str();

    void initiate(std::string date, std::string stime, std::string etime, double _speed);

    void set_start_time();

    double past_milli();
    double past_milli(const FIX::UtcTimeStamp& utc);

    FIX::UtcTimeStamp milli2utc(double milli);
    FIX::UtcTimeStamp utc_now();
};

extern timesetting timepara;
