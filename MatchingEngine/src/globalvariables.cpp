#include "globalvariables.h"

#include <shift/miscutils/terminal/Common.h>

timesetting timepara;

/* static */ boost::posix_time::ptime timesetting::getUTCPTime(const boost::posix_time::ptime& local_pt)
{
    // time_t is always assumed to be UTC,
    // but boost's ptime does not have time zone information,
    // so this is the reason we need all this mess
    time_t local_time_t = boost::posix_time::to_time_t(local_pt);
    tm* local_tm = std::gmtime(&local_time_t);
    time_t utc_time_t = mktime(local_tm);
    tm* utc_tm = std::gmtime(&utc_time_t);

    return boost::posix_time::ptime_from_tm(*utc_tm);
}

void timesetting::initiate(std::string date, std::string stime, double _speed)
{
    cout << endl;

    auto local_date_time = boost::posix_time::ptime(boost::posix_time::time_from_string(date + " " + stime));
    utc_date_time = getUTCPTime(local_date_time);
    cout << "utc_date_time: " << boost::posix_time::to_iso_extended_string(utc_date_time) << endl;

    hhmmss = utc_date_time.time_of_day().total_seconds();
    cout << "hhmmss: " << hhmmss << endl;
    speed = _speed;
}

void timesetting::set_start_time()
{
    std::chrono::high_resolution_clock h_resol_clock; // initiate a high resolution clock
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
    std::chrono::high_resolution_clock h_resol_clock; // initiate a high resolution clock
    std::chrono::high_resolution_clock::time_point local_now = h_resol_clock.now(); // initiate a high resolution time point
    long milli = std::chrono::duration_cast<std::chrono::milliseconds>(local_now - start_time_point).count();
    return (speed * milli);
}

/**
 * @brief Get FIX::UtcTimeStamp from millisecond since server starts 
 */
FIX::UtcTimeStamp timesetting::milli2utc(double milli)
{
    boost::posix_time::ptime now = utc_date_time + boost::posix_time::milliseconds(milli);
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
    auto tm_utc_now = boost::posix_time::to_tm(utc_date_time + micro);
    auto utc_timestamp = FIX::UtcTimeStamp(&tm_utc_now, (int)micro.fractional_seconds(), 6);
    return utc_timestamp;
}
