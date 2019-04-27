#include "TimeSetting.h"

#include <shift/miscutils/terminal/Common.h>

/* static */ TimeSetting& TimeSetting::getInstance()
{
    static TimeSetting s_timeSettingInst;
    return s_timeSettingInst;
}

/**
 * @brief Convert local ptime to UTC ptime
 */
/* static */ boost::posix_time::ptime TimeSetting::getUTCPTime(const boost::posix_time::ptime& localPtime)
{
    // std::time_t is always assumed to be UTC,
    // but boost's ptime does not have time zone information,
    // so this is the reason we need all this mess
    std::time_t ttLocal = boost::posix_time::to_time_t(localPtime);
    std::tm* tmLocal = std::gmtime(&ttLocal);

    // A negative value of time->tm_isdst causes mktime to attempt to determine if DST was in effect
    // More information is available at: https://en.cppreference.com/w/cpp/chrono/c/mktime
    tmLocal->tm_isdst = -1;
    std::time_t ttUtc = mktime(tmLocal);
    std::tm* tmUtc = std::gmtime(&ttUtc);

    return boost::posix_time::ptime_from_tm(*tmUtc);
}

/**
 * @brief Initiate member variables
 */
void TimeSetting::initiate(const boost::posix_time::ptime& localPtime, int speed)
{
    m_utcDateTime = getUTCPTime(localPtime);
    m_hhmmss = m_utcDateTime.time_of_day().total_seconds();
    m_speed = speed;

    cout << endl;
    cout << "UTC date time: " << boost::posix_time::to_iso_extended_string(m_utcDateTime) << endl;
    cout << "HHMMSS: " << m_hhmmss << endl;
    cout << "Speed: " << m_speed << endl;
}

/**
 * @brief Set m_startTimePoint as real local timestamp
 */
void TimeSetting::setStartTime()
{
    std::chrono::high_resolution_clock highRezClock; // initiate a high resolution clock
    m_startTimePoint = highRezClock.now();
}

/**
 * @brief Get total millisecond from now
 */
long TimeSetting::pastMilli(bool simTime) // FIXME: can be replaced by microsecond
{
    std::chrono::high_resolution_clock highRezClock; // initiate a high resolution clock
    std::chrono::high_resolution_clock::time_point localNow = highRezClock.now(); // initiate a high resolution time point
    long milli = std::chrono::duration_cast<std::chrono::milliseconds>(localNow - m_startTimePoint).count();

    return simTime ? (m_speed * milli) : milli;
}

/**
 * @ brief Get total millisecond from FIX::UtcTimeStamp
 */
long TimeSetting::pastMilli(const FIX::UtcTimeStamp& utc, bool simTime)
{
    long milli = (utc.getHour() * 3600 + utc.getMinute() * 60 + utc.getSecond() - m_hhmmss) * 1000 + utc.getMillisecond();

    return simTime ? (m_speed * milli) : milli;
}

/**
 * @brief Get FIX::UtcTimeStamp from now
 */
FIX::UtcTimeStamp TimeSetting::simulationTimestamp()
{
    std::chrono::high_resolution_clock highRezClock;
    std::chrono::high_resolution_clock::time_point timeNow = highRezClock.now();
    boost::posix_time::microseconds micro(std::chrono::duration_cast<std::chrono::microseconds>(timeNow - m_startTimePoint).count() * m_speed);
    auto tmUtcSec = boost::posix_time::to_tm(m_utcDateTime + micro);
    auto timestampUtc = FIX::UtcTimeStamp(&tmUtcSec, (int)micro.fractional_seconds(), 6);

    return timestampUtc;
}
