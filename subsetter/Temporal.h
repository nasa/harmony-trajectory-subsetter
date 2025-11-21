#ifndef Temporal_H
#define Temporal_H

#include "LogLevel.h"
#include <algorithm>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <exception>
#include <iostream>
#include <string>

// class to test whether dataset temporal value is within the range provided by
// user
class Temporal
{
  public:
    /**
     * @brief Construct a new Temporal object using datetime string:
     *        YYYY-MM-DDTHH:MM:SS.ssssss
     * @param s Start time in datetime string format.
     * @param e End time in datetime string format.
     * @param epochTime Reference time of start and end time values.
     */
    Temporal(std::string s, std::string e, std::string epochTime = "")
        : epochUpdateRequired(true)
    {
        LOG_DEBUG("Temporal::Temporal(): ENTER");

        if (s.find(":") == std::string::npos)
            s += " 00:00:00.000000";
        if (e.find(":") == std::string::npos)
            e += " 00:00:00.000000";
        replace(s.begin(), s.end(), 'T', ' ');
        replace(s.begin(), s.end(), 'Z', ' ');
        replace(e.begin(), e.end(), 'T', ' ');
        replace(e.begin(), e.end(), 'Z', ' ');
        myReferenceTime = (epochTime.empty()) ? unixEpochTime : epochTime;
        try
        {
            boost::posix_time::ptime referTime(
                boost::posix_time::time_from_string(myReferenceTime));
            boost::posix_time::ptime startTime(
                boost::posix_time::time_from_string(s));
            boost::posix_time::ptime endTime(
                boost::posix_time::time_from_string(e));
            boost::posix_time::time_duration startSinceReference =
                startTime - referTime;
            boost::posix_time::time_duration endSinceReference =
                endTime - referTime;
            start =
                (double)startSinceReference.total_microseconds() / 1000000.0;
            end = (double)endSinceReference.total_microseconds() / 1000000.0;
        }
        catch (std::exception &ex)
        {
            LOG_ERROR(
                "Temporal::Temporal(): ERROR: Temporal.ctor failed to parse "
                "the time strings "
                << s << " " << e);
            std::exception_ptr p = std::current_exception();
            rethrow_exception(p);
        }
        std::cout.setf(std::ios::fixed); // to print more precision
        LOG_DEBUG("Temporal::Temporal(): start end " << start << " " << end);
        std::cout.unsetf(std::ios_base::floatfield); // set back
    }

    /**
     * @brief Construct a new Temporal object using seconds since epoch.
     *
     *        The reference time has no initial value, because the input
     *        start and end times are already respect to whatever the time
     *        coordinate attributes are. This is also why no epoch update
     *        is required.
     *
     * @param s Start time in seconds since epoch.
     * @param e End time in seconds since epoch.
     */
    Temporal(double s, double e) : epochUpdateRequired(false)
    {
        myReferenceTime = "";
        start = s;
        end = e;
    }

    double getStart() { return this->start; }

    double getEnd() { return this->end; }

    // return the start date time string
    std::string getStartTime() { return convertToDateTimeString(this->start); }

    // return the end date time string
    std::string getEndTime() { return convertToDateTimeString(this->end); }

    // update reference time if it differs from the dataset's epoch.
    // if the epochUpdateRequired is set to false, we don't update.
    bool needToUpdateEpoch(std::string epoch)
    {
        if (myReferenceTime != epoch && epochUpdateRequired)
        {
            return true;
        }
        else if (!epochUpdateRequired)
        {
            replace(epoch.begin(), epoch.end(), 'T', ' ');
            replace(epoch.begin(), epoch.end(), 'Z', ' ');
            myReferenceTime = epoch;
        }

        return false;
    }

    // update the reference time, start and end
    void updateReferenceTime(std::string referenceTime)
    {
        LOG_DEBUG("Temporal::updateReferenceTime(): ENTER");

        // get the difference between old and new reference time
        replace(referenceTime.begin(), referenceTime.end(), 'T', ' ');
        replace(referenceTime.begin(), referenceTime.end(), 'Z', ' ');

        try
        {
            boost::posix_time::time_duration diffTime =
                boost::posix_time::ptime(
                    boost::posix_time::time_from_string(referenceTime)) -
                boost::posix_time::ptime(
                    boost::posix_time::time_from_string(myReferenceTime));
            double diffSeconds =
                (double)diffTime.total_microseconds() / 1000000.0;
            start -= diffSeconds;
            end -= diffSeconds;
            myReferenceTime = referenceTime;
        }
        catch (std::exception &ex)
        {
            LOG_ERROR(
                "ERROR: Temporal.updateReferenceTime failed to parse the time "
                "strings "
                << referenceTime);
            std::exception_ptr p = std::current_exception();
            rethrow_exception(p);
        }
        std::cout.setf(std::ios::fixed); // to print more precision
        LOG_DEBUG("Temporal::updateReferenceTime(): start end " << start << " "
                                                                << end);
        std::cout.unsetf(std::ios_base::floatfield); // set back
    }

    bool contains(double t) { return (t >= start && t <= end); }

  private:
    // convert to the date time std::string in format YYYY-MM-DDTHH:MM:SS.ffffff
    std::string convertToDateTimeString(double timeInSeconds)
    {
        try
        {
            // parse the integer and fractional part
            double intpart;
            double fractpart = modf(timeInSeconds, &intpart);
            boost::posix_time::ptime t =
                boost::posix_time::time_from_string(myReferenceTime) +
                boost::posix_time::seconds((long)intpart) +
                boost::posix_time::microseconds((long)(fractpart * 1000000.0));
            return to_iso_extended_string(t);
        }
        catch (const std::exception &ex)
        {
            LOG_INFO("Temporal::convertToDateTimeString: "
                     << ex.what() << ", setting date time to 00:00:00.000000");
            return std::string("00:00:00.000000");
        }
    }

    // seconds since reference time
    double start;
    double end;
    std::string myReferenceTime;

    // epoch update required depends on constructor.
    // the constructor that takes in datetime start/end arguments will
    // require reference time adjustments, however,
    // the constructor that takes in start/end arguments in seconds
    // have already been created in reference to correct epoch.
    bool epochUpdateRequired;

    // use as default epoch time
    std::string unixEpochTime = "1970-01-01 00:00:00.000000";
};

#endif
