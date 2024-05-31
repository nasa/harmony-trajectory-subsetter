#ifndef Temporal_H
#define Temporal_H

#include <iostream>
#include <string>
#include <algorithm>
#include <exception>
#include <boost/date_time/posix_time/posix_time.hpp>

// class to test whether dataset temporal value is within the range provided by user
class Temporal
{
public:
    // s startTime, e endTime
    Temporal(std::string s, std::string e, std::string epochTime="")
    {
        if (s.find(":") == std::string::npos)
            s += " 00:00:00.000000";
        if (e.find(":") == std::string::npos)
            e += " 00:00:00.000000";
        replace(s.begin(), s.end(), 'T', ' ');
        replace(s.begin(), s.end(), 'Z', ' ');
        replace(e.begin(), e.end(), 'T', ' ');
        replace(e.begin(), e.end(), 'Z', ' ');
        myReferenceTime = (epochTime.empty())? unixEpochTime : epochTime;
        try
        {
            boost::posix_time::ptime referTime(boost::posix_time::time_from_string(myReferenceTime));
            boost::posix_time::ptime startTime(boost::posix_time::time_from_string(s));
            boost::posix_time::ptime endTime(boost::posix_time::time_from_string(e));
            boost::posix_time::time_duration startSinceReference = startTime - referTime;
            boost::posix_time::time_duration endSinceReference = endTime - referTime;
            start = (double)startSinceReference.total_microseconds()/1000000.0;
            end = (double)endSinceReference.total_microseconds()/1000000.0;
        }
        catch (std::exception &ex)
        {
            std::cerr << "ERROR: Temporal.ctor failed to parse the time strings " << s << " " << e << std::endl;
            std::exception_ptr p = std::current_exception();
            rethrow_exception(p);
        }
        std::cout.setf(std::ios::fixed); // to print more precision
        std::cout << "Temporal.ctor Temporal start end " << start << " " << end << std::endl;
        std::cout.unsetf(std::ios_base::floatfield); //set back
    }

    double getStart()
    {
        return this->start;
    }

    double getEnd()
    {
        return this->end;
    }

    // return the start date time string
    std::string getStartTime()
    {
        return convertToDateTimeString(this->start);
    }

    // return the end date time string
    std::string getEndTime()
    {
        return convertToDateTimeString(this->end);
    }

    bool needToUpdateEpoch(std::string epoch)
    {
        return (myReferenceTime != epoch);
    }

    // update the reference time, start and end
    void updateReferenceTime(std::string referenceTime)
    {
        // get the difference between old and new reference time
        replace(referenceTime.begin(), referenceTime.end(), 'T', ' ');
        replace(referenceTime.begin(), referenceTime.end(), 'Z', ' ');
        try
        {
            boost::posix_time::time_duration diffTime = boost::posix_time::ptime(boost::posix_time::time_from_string(referenceTime)) - boost::posix_time::ptime(boost::posix_time::time_from_string(myReferenceTime));
            double diffSeconds = (double)diffTime.total_microseconds()/1000000.0;
            start -= diffSeconds;
            end -= diffSeconds;
            myReferenceTime = referenceTime;
        }
        catch (std::exception &ex)
        {
            std::cerr << "ERROR: Temporal.updateReferenceTime failed to parse the time strings " << referenceTime << std::endl;
            std::exception_ptr p = std::current_exception();
            rethrow_exception(p);
        }
        std::cout.setf(std::ios::fixed); // to print more precision
        std::cout << "Temporal::updateReferenceTime start end " << start << " " << end << std::endl;
        std::cout.unsetf(std::ios_base::floatfield); //set back

    }

    bool contains(double t)
    {
        return (t >= start && t <= end);
    }

private:
    // convert to the date time std::string in format YYYY-MM-DDTHH:MM:SS.ffffff
    std::string convertToDateTimeString(double timeInSeconds)
    {
        // parse the integer and fractional part
        double intpart;
        double fractpart = modf(timeInSeconds, &intpart);
        boost::posix_time::ptime t = boost::posix_time::time_from_string(myReferenceTime) + boost::posix_time::seconds((long)intpart) + boost::posix_time::microseconds((long)(fractpart*1000000.0));
        return to_iso_extended_string(t);
    }

    // seconds since reference time
    double start;
    double end;
    std::string myReferenceTime;

    // use as default epoch time
    static std::string unixEpochTime;

};
std::string Temporal::unixEpochTime = "1970-01-01 00:00:00.000000";

#endif
