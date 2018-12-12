#include "globalvariables.h"

#include <shift/miscutils/terminal/Common.h>

timesetting timepara;

/* static */ std::string timesetting::now_str()
{
    boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
    std::string now_string = boost::posix_time::to_iso_extended_string(now);
    now_string[10] = ' ';
    //std::cout << now_string<<endl;
    return now_string;
}

void timesetting::initiate(std::string date, std::string stime, std::string etime, double _speed)
{
    Innerstartdate = date;
    Innerstarttime = stime;
    Innerendtime = etime;
    std::string timestring = date + " " + stime;
    cout << endl;
    date_time = boost::posix_time::ptime(boost::posix_time::time_from_string(timestring));
    cout << "date_time: " << boost::posix_time::to_iso_extended_string(date_time) << endl;
    hhmmss = date_time.time_of_day().total_seconds();
    cout << "hhmmss: " << hhmmss << endl;
    speed = _speed;
}

void timesetting::set_start_time()
{
    std::chrono::high_resolution_clock h_resol_clock; //initiate a high resolute clock
    start_time_point = h_resol_clock.now();
}

/**
 * @ brief Get total millisecond from FIX::UtcTimeStamp 
 */
double timesetting::past_milli(const FIX::UtcTimeStamp& utc)
{
    double milli = (utc.getHour() * 3600 + utc.getMinute() * 60 + utc.getSecond() - hhmmss) * 1000 + utc.getMillisecond();
    return milli * speed;
}

/**
 * @brief Get total millisecond from now
 */
double timesetting::past_milli() // FIXME: can be replaced by microsecond
{
    std::chrono::high_resolution_clock h_resol_clock; // initiate a high resolute clock
    std::chrono::high_resolution_clock::time_point timenow = h_resol_clock.now(); // initiate a high resolute time point
    long milli = std::chrono::duration_cast<std::chrono::milliseconds>(timenow - start_time_point).count();
    return (speed * milli);
}

/**
 * @brief Get FIX::UtcTimeStamp from millisecond since server starts 
 */
FIX::UtcTimeStamp timesetting::milli2utc(double milli)
{
    boost::posix_time::ptime now = date_time + boost::posix_time::milliseconds(milli);
    auto tm_now = boost::posix_time::to_tm(now);
    auto utc_now = FIX::UtcTimeStamp(&tm_now, ((int)milli % 1000) * 1000, 6);
    return utc_now;
}

/**
 * @brief Get FIX::UtcTimeStamp from now
 */
FIX::UtcTimeStamp timesetting::utc_now()
{
    std::chrono::high_resolution_clock h_resol_clock;
    std::chrono::high_resolution_clock::time_point timenow = h_resol_clock.now();
    boost::posix_time::microseconds micro(std::chrono::duration_cast<std::chrono::microseconds>(timenow - start_time_point).count() * speed);
    boost::posix_time::ptime now = date_time + micro;
    auto tm_now = boost::posix_time::to_tm(now);
    auto utc_now = FIX::UtcTimeStamp(&tm_now, (int)micro.fractional_seconds(), 6);
    return utc_now;
}
