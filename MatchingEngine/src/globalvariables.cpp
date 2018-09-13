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
    date_time = boost::posix_time::ptime(boost::posix_time::time_from_string(timestring));
    cout << "Date_time: " << boost::posix_time::to_iso_extended_string(date_time) << endl;
    hhmmss = date_time.time_of_day().total_seconds();
    cout << "hhmmss: " << hhmmss << endl;
    speed = _speed;
}

void timesetting::set_start_time()
{
    std::chrono::high_resolution_clock h_resol_clock; //initiate a high resolute clock
    start_time_point = h_resol_clock.now();
}

long timesetting::past_milli(std::string& _date_time)
{
    //    boost::posix_time::ptime p_date_time(boost::posix_time::time_from_string(_date_time));
    //    boost::posix_time::time_duration timelaps=p_date_time-date_time;
    //    return(timelaps.total_milliseconds());
    std::stringstream ss(_date_time);
    ss.ignore(11); //throw away date
    long milli_second;
    double digit;
    ss >> digit; //get hours
    milli_second = digit * 60; //convert to minutes
    ss.ignore(1); //throw ':'
    ss >> digit; //get minutes
    milli_second += digit; //sum minutes with hours
    ss.ignore(1);
    ss >> digit; //get the rest, seconds and decimal: ss.000000
    milli_second = (milli_second * 60 - hhmmss + digit) * 1000; //calculate the diff with start time
    return milli_second;
}

double timesetting::past_milli()
{
    std::chrono::high_resolution_clock::time_point timenow; //initiate a high resolute time point
    std::chrono::high_resolution_clock h_resol_clock; //initiate a high resolute clock
    timenow = h_resol_clock.now(); //use the clock to get high resolute time and store in time point
    long milli = std::chrono::duration_cast<std::chrono::milliseconds>(timenow - start_time_point).count();
    return (speed * milli);
}

std::string timesetting::milli2str(double milli)
{
    boost::posix_time::ptime now = date_time + boost::posix_time::milliseconds(milli);
    std::string timestamp = boost::posix_time::to_iso_extended_string(now);
    timestamp[10] = ' ';
    return timestamp;
}

std::string timesetting::timestamp_inner()
{
    std::chrono::high_resolution_clock::time_point timenow; //initiate a high resolute time point
    std::chrono::high_resolution_clock h_resol_clock; //initiate a high resolute clock
    timenow = h_resol_clock.now(); //use the clock to get high resolute time and store in time point
    boost::posix_time::microseconds micro(std::chrono::duration_cast<std::chrono::microseconds>(timenow - start_time_point).count() * speed);
    boost::posix_time::ptime now = date_time + micro;
    std::string timestamp = boost::posix_time::to_iso_extended_string(now);
    timestamp[10] = ' ';
    return timestamp;
}
