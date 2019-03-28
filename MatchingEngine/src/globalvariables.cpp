#include "globalvariables.h"

#include <shift/miscutils/terminal/Common.h>

timesetting timepara;

/**
 * @brief Convert local ptime to UTC ptime
 */
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

/**
 * @brief initiate member variables
 */
void timesetting::initiate(std::string date, std::string stime, int speed)
{
    cout << endl;

    auto local_date_time = boost::posix_time::ptime(boost::posix_time::time_from_string(date + " " + stime));
    m_utcDateTime = getUTCPTime(local_date_time);
    cout << "UTC date time: " << boost::posix_time::to_iso_extended_string(m_utcDateTime) << endl;

    m_hhmmss = m_utcDateTime.time_of_day().total_seconds();
    cout << "HHMMSS: " << m_hhmmss << endl;
    m_speed = speed;
    cout << "Speed: " << m_speed << endl;
}

/**
 * @brief Set m_startTimePoint as real local timestamp
 */
void timesetting::setStartTime()
{
    std::chrono::high_resolution_clock h_resol_clock; // initiate a high resolution clock
    m_startTimePoint = h_resol_clock.now();
}

/**
 * @brief Get total millisecond from now
 */
long timesetting::pastMilli(bool simulation_time) // FIXME: can be replaced by microsecond
{
    std::chrono::high_resolution_clock h_resol_clock; // initiate a high resolution clock
    std::chrono::high_resolution_clock::time_point local_now = h_resol_clock.now(); // initiate a high resolution time point
    long milli = std::chrono::duration_cast<std::chrono::milliseconds>(local_now - m_startTimePoint).count();
    return simulation_time ? (m_speed * milli) : milli;
}

/**
 * @ brief Get total millisecond from FIX::UtcTimeStamp
 */
long timesetting::pastMilli(const FIX::UtcTimeStamp& utc, bool simulation_time)
{
    long milli = (utc.getHour() * 3600 + utc.getMinute() * 60 + utc.getSecond() - m_hhmmss) * 1000 + utc.getMillisecond();
    return simulation_time ? (m_speed * milli) : milli;
}

/**
 * @brief Get FIX::UtcTimeStamp from now
 */
FIX::UtcTimeStamp timesetting::simulationTimestamp()
{
    std::chrono::high_resolution_clock h_resol_clock;
    std::chrono::high_resolution_clock::time_point timenow = h_resol_clock.now();
    boost::posix_time::microseconds micro(
        std::chrono::duration_cast<std::chrono::microseconds>(timenow - m_startTimePoint).count() * m_speed);
    auto tm_utc_now = boost::posix_time::to_tm(m_utcDateTime + micro);
    auto utc_timestamp = FIX::UtcTimeStamp(&tm_utc_now, (int)micro.fractional_seconds(), 6);
    return utc_timestamp;
}
