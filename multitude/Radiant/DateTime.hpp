/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_DATETIME_HPP
#define RADIANT_DATETIME_HPP

#include "Export.hpp"
#include "TimeStamp.hpp"

namespace Radiant {

  /** Combination of date and time information. */

  class RADIANT_API DateTime
  {
  public:

    /// Format for date & time
    enum DateFormat {
      /// Date and time in ISO format
      DATE_TIME_ISO,
      /// Just the date in ISO format
      DATE_ISO
    };

    DateTime();
    /// Constructs a copy
    DateTime(const TimeStamp & );
    ~DateTime();

    /// Year (anno domini)
    int year() const { return m_year; }
    /// Month of year (January = 0)
    int month() const { return m_month; }
    /// Day of month (0-30)
    int monthDay() const { return m_monthDay; }
    /// Day of week (Sunday = 0)
    int weekDay() const { return m_weekDay; }
    /// Hour since midnight (0-23)
    int hour() const { return m_hour; }
    /// Minutes since last full hour (0-59)
    int minute() const { return m_minute; }
    /// Seconds since last full minute (0-59)
    int second() const { return m_second; }
    /// Milliseconds since last full second (0-999)
    int milliSecond() const { return m_microsecond / 1000; }
    /// Microseconds since last full second (0-999999)
    int microSecond() const { return m_microsecond; }
    /// Reset the hour, minute and second values to zero
    void clearTime();

    /// Set the year
    void setYear(int year) { m_year = year; }
    /// Set the month
    void setMonth(int month) { m_month = month; }
    /// Set the day of the month
    void setMonthDay(int monthDay) { m_monthDay = monthDay; }
    /// Set the day of the week
    void setWeekDay(int weekDay) { m_weekDay = weekDay; }
    /// Set the hour
    void setHour(int hour) { m_hour = hour; }
    /// Set the minute
    void setMinute(int minute) { m_minute = minute; }
    /// Set the second
    void setSecond(int second) { m_second = second; }

    /// Advance time to next year
    void toNextYear();
    /// Advance time to next month
    void toNextMonth();
    /// Advance time to next day of the month
    void toNextMonthDay();
    /// Read time and date from a string
    bool fromString(const QString & s, DateFormat format = DATE_ISO);

    /** Returns the number of days in the month. This function does
    take the leap years into account, so the length of Febuary changes
    between 28 and 29 days, depending on the year.
    @param month month [0-11]
    @param year year number
    @return number of days in the month
    */
    static int daysInMonth(int month, int year);

    /** Returns the number of days in the current month. This method
    takes the leap-years into account.
    @return number of days in the month*/
    int daysInMonth();

    /// Return the date and time as a TimeStamp
    /// @return the date as time-stamp
    TimeStamp asTimeStamp() const;

    /// Print the date-time information to a string.
    /// @param[out] buf buffer to write to
    /// @param isotime use ISO time format
    void print(char * buf, bool isotime = false);

  private:
    int m_year;
    int m_month;
    int m_monthDay;
    int m_weekDay;
    int m_hour;
    int m_minute;
    int m_second;
    int m_microsecond;
    bool m_summerTime;
  };
}

#endif
