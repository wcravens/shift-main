#include "globalvariables.h"

#include <shift/miscutils/terminal/Common.h>

timesetting timepara;

/* static */ std::string timesetting::now_str()
{
    boost::posix_time::ptime now(boost::posix_time::microsec_clock::universal_time());
    // boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());
    std::string now_string = boost::posix_time::to_iso_extended_string(now);
    now_string[10] = ' ';
    // std::cout << "Time now: " << now_string << std::endl;
    return now_string;
}

/* static */ boost::posix_time::ptime
timesetting::getUtcPtime(const boost::posix_time::ptime& local_pt)
{
    // Grab copies of the current time in local and UTC form.
    auto utc_time = boost::posix_time::microsec_clock::universal_time(); // UTC.
    auto utc_time_t = boost::posix_time::to_time_t(utc_time);
    auto* local_tm = std::localtime(&utc_time_t);
    auto local_time = boost::posix_time::ptime_from_tm(*local_tm);

    // Return the given time in ms plus the time_t difference
    return local_pt + (utc_time - local_time);
}

void timesetting::initiate(std::string date, std::string stime, std::string etime, double _speed)
{
    Innerstartdate = date;
    Innerstarttime = stime;
    Innerendtime = etime;
    std::string timestring = date + " " + stime;
    cout << endl;

    auto local_date_time = boost::posix_time::ptime(boost::posix_time::time_from_string(timestring));
    utc_date_time = getUtcPtime(local_date_time);
    cout << "utc_date_time: " << boost::posix_time::to_iso_extended_string(utc_date_time) << endl;

    hhmmss = utc_date_time.time_of_day().total_seconds();
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
    std::chrono::high_resolution_clock::time_point local_now = h_resol_clock.now(); // initiate a high resolute time point
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
