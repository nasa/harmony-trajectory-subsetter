#ifndef Temporal_H
#define Temporal_H

#include <iostream>
#include <string>
#include <algorithm>
#include <exception>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::posix_time;

// class to test whether dataset temporal value is within the range provided by user
class Temporal
{
public:
    // s startTime, e endTime
    Temporal(string s, string e, string epochTime="")
    {
        if (s.find(":") == string::npos)
            s += " 00:00:00.000000";
        if (e.find(":") == string::npos)
            e += " 00:00:00.000000";
        replace(s.begin(), s.end(), 'T', ' ');
        replace(s.begin(), s.end(), 'Z', ' ');
        replace(e.begin(), e.end(), 'T', ' ');
        replace(e.begin(), e.end(), 'Z', ' ');        
        myReferenceTime = (epochTime.empty())? unixEpochTime : epochTime;
        try
        {
            ptime referTime(time_from_string(myReferenceTime));
            ptime startTime(time_from_string(s));
            ptime endTime(time_from_string(e));
            time_duration startSinceReference = startTime - referTime;
            time_duration endSinceReference = endTime - referTime;
            start = (double)startSinceReference.total_microseconds()/1000000.0;
            end = (double)endSinceReference.total_microseconds()/1000000.0;
        }
        catch (std::exception &ex)
        {
            cerr << "ERROR: Temporal.ctor failed to parse the time strings " << s << " " << e << endl;
            std::exception_ptr p = current_exception();
            rethrow_exception(p);
        }
        cout.setf(ios::fixed); // to print more precision
        cout << "Temporal.ctor Temporal start end " << start << " " << end << endl;
        cout.unsetf(ios_base::floatfield); //set back
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
    string getStartTime() 
    {
        return convertToDateTimeString(this->start);
    }

    // return the end date time string 
    string getEndTime()
    {
        return convertToDateTimeString(this->end); 
    } 

    bool needToUpdateEpoch(string epoch) 
    { 
        return (myReferenceTime != epoch);
    }
    
    // update the reference time, start and end 
    void updateReferenceTime(string referenceTime)
    {
        // get the difference between old and new reference time
        replace(referenceTime.begin(), referenceTime.end(), 'T', ' ');
        replace(referenceTime.begin(), referenceTime.end(), 'Z', ' ');
        try
        {
            time_duration diffTime = ptime(time_from_string(referenceTime)) - ptime(time_from_string(myReferenceTime));
            double diffSeconds = (double)diffTime.total_microseconds()/1000000.0;
            start -= diffSeconds;
            end -= diffSeconds;
            myReferenceTime = referenceTime;
        }
        catch (std::exception &ex)
        {
            cerr << "ERROR: Temporal.updateReferenceTime failed to parse the time strings " << referenceTime << endl;
            std::exception_ptr p = current_exception();
            rethrow_exception(p);
        }
        cout.setf(ios::fixed); // to print more precision
        cout << "Temporal::updateReferenceTime start end " << start << " " << end << endl;
        cout.unsetf(ios_base::floatfield); //set back

    }

    bool contains(double t)
    {
        return (t >= start && t <= end);
    }

private:
    // convert to the date time string in format YYYY-MM-DDTHH:MM:SS.ffffff
    string convertToDateTimeString(double timeInSeconds)
    {
        // parse the integer and fractional part
        double intpart;
        double fractpart = modf(timeInSeconds, &intpart);
        ptime t = time_from_string(myReferenceTime) + seconds((long)intpart) + microseconds((long)(fractpart*1000000.0));
        return to_iso_extended_string(t);
    }

    // seconds since reference time
    double start;
    double end;
    string myReferenceTime;
    
    // use as default epoch time
    static string unixEpochTime;
    
};
string Temporal::unixEpochTime = "1970-01-01 00:00:00.000000";

#endif
