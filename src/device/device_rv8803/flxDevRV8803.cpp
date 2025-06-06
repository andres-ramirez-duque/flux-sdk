/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2024, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

/*
 *
 *  flxDevRV8803.cpp
 *
 *  Spark Device object for the RV8803 device.
 *
 *
 */

#include "Arduino.h"

#include "flxDevRV8803.h"

uint8_t flxDevRV8803::defaultDeviceAddress[] = {RV8803_ADDR, kSparkDeviceAddressNull};

// Register this class with the system - this enables the *auto load* of this device
flxRegisterDevice(flxDevRV8803);

flxDevRV8803::flxDevRV8803()
{

    setName(getDeviceName(), "RV8803 Real Time Clock");

    // Register output params
    flxRegister(readDateUSA, "Read Date (MM/DD/YYYY)", "Read the date in USA format");
    flxRegister(readDate, "Read Date (DD/MM/YY)", "Read the date");
    flxRegister(readTime, "Read Time (HH:MM:SS)", "Read the time");
    flxRegister(readISO8601, "Read ISO8601 DateTime (YYYY-MM-DDTHH:MM:SS)", "Read the date and time in ISO8601 format");
    flxRegister(readISO8601TZ, "Read ISO8601 DateTime (YYYY-MM-DDTHH:MM:SS+HH:MM)",
                "Read the date and time in ISO8601 format with time zone");
    flxRegister(readDayOfWeek, "Read the day of week", "Read the day of week (Monday, Tuesday, etc)");
    flxRegister(readDayOfWeekShort, "Read the day of week (short)", "Read the day of week (Mon, Tue, etc)");
    flxRegister(readOrdinal, "Read the day of month (ordinal)",
                "Read the day of month in ordinal form (1st, 2nd, 3rd etc)");
    flxRegister(readMonth, "Read the month", "Read the name of the month (January, February etc)");
    flxRegister(readMonthShort, "Read the month (short)", "Read the name of the month (Jan, Feb etc)");
    flxRegister(readYear, "Read the year", "Read the year (2022 etc)");
    flxRegister(getEpoch, "Get Epoch", "Get the time in seconds since the Epoch", kParamValueEpoch);

    // Register input params
    flxRegister(setEpoch, "Set Epoch", "Set the time since the Epoch");
    flxRegister(setSeconds, "Set the seconds", "Set the seconds");
    flxRegister(setMinutes, "Set the minutes", "Set the minutes");
    flxRegister(setHours, "Set the hours", "Set the hours");
    flxRegister(setDate, "Set the date", "Set the date");
    flxRegister(setWeekday, "Set the weekday", "Set the weekday: 0=Sunday, 6=Saturday");
    flxRegister(setMonth, "Set the month", "Set the month");
    flxRegister(setYear, "Set the year", "Set the year");
}

// Static method used to determine if this device is connected

bool flxDevRV8803::isConnected(flxBusI2C &i2cDriver, uint8_t address)
{
    // For speed, ping the device address first
    if (!i2cDriver.ping(address))
        return false;

    // Read the RAM byte. Change it. Check it. Restore it
    uint8_t ramByte = 0;
    if (!i2cDriver.readRegister(address, RV8803_RAM, &ramByte))
        return false;

    if (!i2cDriver.writeRegister(address, RV8803_RAM, 0xAA))
        return false;

    uint8_t ramByte2 = 0;
    if (!i2cDriver.readRegister(address, RV8803_RAM, &ramByte2))
        return false;

    if (!i2cDriver.writeRegister(address, RV8803_RAM, ramByte))
        return false;

    return (ramByte2 == 0xAA);
}

//----------------------------------------------------------------------------------------------------------
// onInitialize()
//
// Called during the startup/initialization of the driver (after the constructor is called).
//
// Place to initialized the underlying device library/driver
//
bool flxDevRV8803::onInitialize(TwoWire &wirePort)
{
    // set the underlying drivers address to the one determined during
    // device construction
    bool result = RV8803::begin(wirePort);

    RV8803::set24Hour();
    RV8803::setTimeZoneQuarterHours(0); // Make sure the time zone is zero

    if (!result)
        flxLog_E("RV8803 - begin failed");

    return result;
}

// Output parameters
std::string flxDevRV8803::read_date_USA()
{
    if (!_dateUSA)
    {
        RV8803::updateTime();
        _date = true;
        _time = true;
        _iso8601 = true;
        _iso8601tz = true;
        _epoch = true;
        _day = true;
        _day_short = true;
        _ordinal = true;
        _month = true;
        _month_short = true;
        _year = true;
    }
    _dateUSA = false;

    char szBuffer[12] = {'\0'};

    if (RV8803::stringDateUSA(szBuffer, sizeof(szBuffer)) != szBuffer) // Should return MM/DD/YYYY
        flxLog_E("RV8803 - read_date_USA failed");

    std::string theString = szBuffer;

    return theString;
}
std::string flxDevRV8803::read_date()
{
    if (!_date)
    {
        RV8803::updateTime();
        _dateUSA = true;
        _time = true;
        _iso8601 = true;
        _iso8601tz = true;
        _epoch = true;
        _day = true;
        _day_short = true;
        _ordinal = true;
        _month = true;
        _month_short = true;
        _year = true;
    }
    _date = false;

    char szBuffer[12] = {'\0'};

    if (RV8803::stringDate(szBuffer, sizeof(szBuffer)) != szBuffer) // Should return DD/MM/YYYY
        flxLog_E("RV8803 - read_date failed");

    std::string theString = szBuffer;

    return theString;
}
std::string flxDevRV8803::read_time()
{
    if (!_time)
    {
        RV8803::updateTime();
        _dateUSA = true;
        _date = true;
        _iso8601 = true;
        _iso8601tz = true;
        _epoch = true;
        _day = true;
        _day_short = true;
        _ordinal = true;
        _month = true;
        _month_short = true;
        _year = true;
    }
    _time = false;

    char szBuffer[12] = {'\0'};

    if (RV8803::stringTime(szBuffer, sizeof(szBuffer)) != szBuffer) // Should return HH:MM:SS
        flxLog_E("RV8803 - read_time failed");

    std::string theString = szBuffer;

    return theString;
}
std::string flxDevRV8803::read_iso8601()
{
    if (!_iso8601)
    {
        RV8803::updateTime();
        _dateUSA = true;
        _date = true;
        _time = true;
        _iso8601tz = true;
        _epoch = true;
        _day = true;
        _day_short = true;
        _ordinal = true;
        _month = true;
        _month_short = true;
        _year = true;
    }
    _iso8601 = false;

    char szBuffer[21] = {'\0'};

    if (RV8803::stringTime8601(szBuffer, sizeof(szBuffer)) != szBuffer) // Should return YYYY-MM-DDTHH:MM:SS
        flxLog_E("RV8803 - read_iso8601 failed");

    std::string theString = szBuffer;

    return theString;
}
std::string flxDevRV8803::read_iso8601_tz()
{
    if (!_iso8601tz)
    {
        RV8803::updateTime();
        _dateUSA = true;
        _date = true;
        _time = true;
        _iso8601 = true;
        _epoch = true;
        _day = true;
        _day_short = true;
        _ordinal = true;
        _month = true;
        _month_short = true;
        _year = true;
    }
    _iso8601tz = false;

    char szBuffer[27] = {'\0'};

    if (RV8803::stringTime8601TZ(szBuffer, sizeof(szBuffer)) != szBuffer) // Should return YYYY-MM-DDTHH:MM:SS-HH:MM
        flxLog_E("RV8803 - read_iso8601 failed");

    std::string theString = szBuffer;

    return theString;
}
std::string flxDevRV8803::read_day()
{
    if (!_day)
    {
        RV8803::updateTime();
        _dateUSA = true;
        _date = true;
        _time = true;
        _iso8601 = true;
        _iso8601tz = true;
        _epoch = true;
        _day_short = true;
        _ordinal = true;
        _month = true;
        _month_short = true;
        _year = true;
    }
    _day = false;

    char szBuffer[11] = {'\0'};

    if (RV8803::stringDayOfWeek(szBuffer, sizeof(szBuffer)) != szBuffer)
        flxLog_E("RV8803 - read_day failed");

    std::string theString = szBuffer;

    return theString;
}
std::string flxDevRV8803::read_day_short()
{
    if (!_day_short)
    {
        RV8803::updateTime();
        _dateUSA = true;
        _date = true;
        _time = true;
        _iso8601 = true;
        _iso8601tz = true;
        _epoch = true;
        _day = true;
        _ordinal = true;
        _month = true;
        _month_short = true;
        _year = true;
    }
    _day_short = false;

    char szBuffer[5] = {'\0'};

    if (RV8803::stringDayOfWeekShort(szBuffer, sizeof(szBuffer)) != szBuffer)
        flxLog_E("RV8803 - read_day_short failed");

    std::string theString = szBuffer;

    return theString;
}
std::string flxDevRV8803::read_ordinal()
{
    if (!_ordinal)
    {
        RV8803::updateTime();
        _dateUSA = true;
        _date = true;
        _time = true;
        _iso8601 = true;
        _iso8601tz = true;
        _epoch = true;
        _day = true;
        _day_short = true;
        _month = true;
        _month_short = true;
        _year = true;
    }
    _ordinal = false;

    char szBuffer[6] = {'\0'};

    if (RV8803::stringDateOrdinal(szBuffer, sizeof(szBuffer)) != szBuffer)
        flxLog_E("RV8803 - read_ordinal failed");

    std::string theString = szBuffer;

    return theString;
}
std::string flxDevRV8803::read_month()
{
    if (!_month)
    {
        RV8803::updateTime();
        _dateUSA = true;
        _date = true;
        _time = true;
        _iso8601 = true;
        _iso8601tz = true;
        _epoch = true;
        _day = true;
        _day_short = true;
        _ordinal = true;
        _month_short = true;
        _year = true;
    }
    _month = false;

    char szBuffer[11] = {'\0'};

    if (RV8803::stringMonth(szBuffer, sizeof(szBuffer)) != szBuffer)
        flxLog_E("RV8803 - read_month failed");

    std::string theString = szBuffer;

    return theString;
}
std::string flxDevRV8803::read_month_short()
{
    if (!_month_short)
    {
        RV8803::updateTime();
        _dateUSA = true;
        _date = true;
        _time = true;
        _iso8601 = true;
        _iso8601tz = true;
        _epoch = true;
        _day = true;
        _day_short = true;
        _ordinal = true;
        _month = true;
        _year = true;
    }
    _month_short = false;

    char szBuffer[5] = {'\0'};

    if (RV8803::stringMonthShort(szBuffer, sizeof(szBuffer)) != szBuffer)
        flxLog_E("RV8803 - read_month_short failed");

    std::string theString = szBuffer;

    return theString;
}
uint16_t flxDevRV8803::read_year()
{
    if (!_year)
    {
        RV8803::updateTime();
        _dateUSA = true;
        _date = true;
        _time = true;
        _iso8601 = true;
        _iso8601tz = true;
        _epoch = true;
        _day = true;
        _day_short = true;
        _ordinal = true;
        _month = true;
        _month_short = true;
        _year = true;
    }
    _year = false;

    return RV8803::getYear();
}
uint32_t flxDevRV8803::get_epoch()
{
    if (!_epoch)
    {
        RV8803::updateTime();
        _dateUSA = true;
        _date = true;
        _time = true;
        _iso8601 = true;
        _iso8601tz = true;
        _day = true;
        _day_short = true;
        _ordinal = true;
        _month = true;
        _month_short = true;
        _year = true;
    }
    _epoch = false;

    return RV8803::getEpoch();
}

bool flxDevRV8803::valid_epoch(void)
{
    return isInitialized();
}
// Output parameters

// Note, this also fulfills the clock inteface needs.
void flxDevRV8803::set_epoch(const uint32_t &epoch)
{
    if (!RV8803::setEpoch(epoch))
        flxLog_E("RV8803 - set_epoch failed");
}

void flxDevRV8803::set_seconds(const uint8_t &secs)
{
    if (!RV8803::setSeconds(secs))
        flxLog_E("RV8803 - set_seconds failed");
}
void flxDevRV8803::set_minutes(const uint8_t &min)
{
    if (!RV8803::setMinutes(min))
        flxLog_E("RV8803 - set_minutes failed");
}
void flxDevRV8803::set_hours(const uint8_t &hour)
{
    if (!RV8803::setHours(hour))
        flxLog_E("RV8803 - set_hour failed");
}
void flxDevRV8803::set_date(const uint8_t &date)
{
    if (!RV8803::setDate(date))
        flxLog_E("RV8803 - set_date failed");
}
void flxDevRV8803::set_month(const uint8_t &month)
{
    if (!RV8803::setMonth(month))
        flxLog_E("RV8803 - set_month failed");
}
void flxDevRV8803::set_year(const uint16_t &year)
{
    if (!RV8803::setYear(year))
        flxLog_E("RV8803 - set_year failed");
}
void flxDevRV8803::set_weekday(const uint8_t &dow)
{
    if (!RV8803::setWeekday(dow))
        flxLog_E("RV8803 - set_weekday failed");
}
