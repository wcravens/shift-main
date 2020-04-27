#include "TimeSetting.h"

#include <shift/miscutils/terminal/Common.h>

/* static */ auto TimeSetting::getInstance() -> TimeSetting&
{
    static TimeSetting s_timeSettingInst;
    return s_timeSettingInst;
}

/**
 * @brief Convert local ptime to UTC ptime.
 */
/* static */ auto TimeSetting::getUTCPTime(const boost::posix_time::ptime& localPtime) -> boost::posix_time::ptime
{
    // std::time_t is always assumed to be UTC,
    // but boost's ptime does not have time zone information,
    // so this is the reason we need all this mess
    std::time_t ttLocal = boost::posix_time::to_time_t(localPtime);
    std::tm* tmLocal = std::gmtime(&ttLocal);

    // a negative value of time->tm_isdst causes mktime to attempt to determine if DST was in effect --
    // more information is available at: https://en.cppreference.com/w/cpp/chrono/c/mktime
    tmLocal->tm_isdst = -1;
    std::time_t ttUtc = mktime(tmLocal);
    std::tm* tmUtc = std::gmtime(&ttUtc);

    return boost::posix_time::ptime_from_tm(*tmUtc);
}

/**
 * @brief Initiate member variables.
 */
void TimeSetting::initiate(const boost::posix_time::ptime& localPtime, int speed)
{
    m_utcDateTime = getUTCPTime(localPtime);
    m_hhmmss = m_utcDateTime.time_of_day().total_seconds();
    m_speed = speed;

    cout << "UTC date and time: " << boost::posix_time::to_iso_extended_string(m_utcDateTime) << endl;
    cout << "Seconds since 00:00: " << m_hhmmss << endl;
    cout << "Simulation speed: " << m_speed << endl;
}

/**
 * @brief Set m_startTimePoint as real local timestamp.
 */
void TimeSetting::setStartTime()
{
    m_startTimePoint = std::chrono::high_resolution_clock::now();
}

/**
 * @brief Get total millisecond from now.
 */
auto TimeSetting::pastMilli(bool simTime) const -> long // FIXME: can be replaced with microsecond version
{
    std::chrono::high_resolution_clock::time_point localNow = std::chrono::high_resolution_clock::now();
    long milli = std::chrono::duration_cast<std::chrono::milliseconds>(localNow - m_startTimePoint).count();

    return simTime ? (m_speed * milli) : milli;
}

/**
 * @ brief Get total millisecond from FIX::UtcTimeStamp.
 */
auto TimeSetting::pastMilli(const FIX::UtcTimeStamp& utc, bool simTime) const -> long // FIXME: can be replaced with microsecond version
{
    long milli = (utc.getHour() * 3600 + utc.getMinute() * 60 + utc.getSecond() - m_hhmmss) * 1000 + utc.getMillisecond();

    return simTime ? (m_speed * milli) : milli;
}

/**
 * @brief Get FIX::UtcTimeStamp from now.
 */
auto TimeSetting::simulationTimestamp() -> FIX::UtcTimeStamp
{
    std::chrono::high_resolution_clock::time_point timeNow = std::chrono::high_resolution_clock::now();
    boost::posix_time::microseconds micro(std::chrono::duration_cast<std::chrono::microseconds>(timeNow - m_startTimePoint).count() * m_speed);
    auto tmUtcSec = boost::posix_time::to_tm(m_utcDateTime + micro);
    auto timestampUtc = FIX::UtcTimeStamp(&tmUtcSec, (int)micro.fractional_seconds(), 6);

    return timestampUtc;
}
