/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2023, SparkFun Electronics Inc.  All rights reserved.
 * This software includes information which is proprietary to and a
 * trade secret of SparkFun Electronics Inc.  It is not to be disclosed
 * to anyone outside of this organization. Reproduction by any means
 * whatsoever is  prohibited without express written permission.
 * 
 *---------------------------------------------------------------------------------
 */
 
/*
 *
 *  flxDevGNSS.h
 *
 *  Spark Device object for the GNSS device.
 *
 *
 *
 */
#include "Arduino.h"

#include "flxDevGNSS.h"

// Define our class static variables - allocs storage for them

#define kGNSSAddressDefault 0x42 // GNSS_ADDR

uint8_t flxDevGNSS::defaultDeviceAddress[] = {kGNSSAddressDefault, kSparkDeviceAddressNull};

//----------------------------------------------------------------------------------------------------------
// Register this class with the system, enabling this driver during system
// initialization and device discovery.

flxRegisterDevice(flxDevGNSS);

//----------------------------------------------------------------------------------------------------------
// Constructor
//
// Object constructor. Performs initialization of device values, including device identifiers (name, I2C address),
// and managed properties.

flxDevGNSS::flxDevGNSS()
{

    // Setup unique identifiers for this device and basic device object systems
    setName(getDeviceName());
    setDescription("u-blox GNSS");

    // Register parameters
    flxRegister(year, "Year", "Year");
    flxRegister(month, "Month", "Month");
    flxRegister(day, "Day", "Day Of Month");
    flxRegister(hour, "Hour", "Hour");
    flxRegister(min, "Minute", "Minute");
    flxRegister(sec, "Second", "Second");
    flxRegister(latitude, "Latitude (deg)", "Latitude in degrees");
    latitude.setPrecision(7);
    flxRegister(longitude, "Longitude (deg)", "Longitude in degrees");
    longitude.setPrecision(7);
    flxRegister(altitude, "Altitude (m)", "Altitude above geoid in metres");
    flxRegister(altitudeMSL, "Altitude MSL (m)", "Altitude above Mean Sea Level in metres");
    flxRegister(SIV, "SIV", "Satellites In View");
    flxRegister(fixType, "Fix Type", "Fix Type");
    flxRegister(carrierSolution, "Carrier Solution", "Carrier Solution");
    flxRegister(groundSpeed, "Ground Speed (m/s)", "Ground speed in metres per second");
    flxRegister(heading, "Heading (deg)", "Heading / course in degrees");
    flxRegister(PDOP, "PDOP", "Position Dilution Of Precision");
    flxRegister(horizontalAccEst, "Horizontal Accuracy Estimate (m)", "Horizontal Position Accuracy Estimate");
    flxRegister(verticalAccEst, "Vertical Accuracy Estimate (m)", "Vertical Position Accuracy Estimate");
    flxRegister(TOW, "TOW (ms)", "Time Of Week in milliseconds");
    flxRegister(iso8601, "ISO8601", "Date and time in ISO 8601 format");
    flxRegister(YYYYMMDD, "YYYYMMDD", "Year/Month/Date");
    flxRegister(YYYYDDMM, "YYYYDDMM", "Year/Date/Month");
    flxRegister(DDMMYYYY, "DDMMYYYY", "Date/Month/Year");
    flxRegister(HHMMSS, "HHMMSS", "Hour:Minute:Second");
    flxRegister(fixTypeStr, "Fix Type (String)", "Fix type in string format");
    flxRegister(carrierSolutionStr, "Carrier Solution (String)", "Carrier solution in string format");

    // Register read-write properties
    flxRegister(measurementRate, "Measurement Rate (ms)", "Set the measurement interval in milliseconds");

    // Register our input parameters
    flxRegister(factoryDefault, "Restore Factory Defaults", "Restore Factory Defaults - takes 5 seconds");
}

//----------------------------------------------------------------------------------------------------------
// Static method used to determine if devices is connected before creating this object (if creating dynamically)
bool flxDevGNSS::isConnected(flxBusI2C &i2cDriver, uint8_t address)
{
    // For speed, ping the device address first
    if (!i2cDriver.ping(address))
        return false;

    uint16_t firstBufferWaiting = 0;
    uint16_t bufferWaiting = 0;
    bool trafficSeen = false;
    unsigned long startTime = millis();

    // Read the number of bytes waiting in the module's I2C buffer
    bool i2cOK = i2cDriver.readRegister16(address, 0xFD, &firstBufferWaiting, false); // Big Endian

    if (!i2cOK)
        return false; // Return now of the read failed

    // Wait for up to 2 seconds for more bytes to be added
    while(i2cOK && (!trafficSeen) && (millis() < (startTime + 2000)))
    {
        delay(100); // Don't pound the bus
        i2cOK = i2cDriver.readRegister16(address, 0xFD, &bufferWaiting, false); // Big Endian
        if (i2cOK)
            trafficSeen = (bufferWaiting != firstBufferWaiting); // Has traffic been seen?
    }

    if (i2cOK && trafficSeen)
        return true; // Return now if traffic has been seen

    if (!i2cOK)
        flxLog_E("GNSS::isConnected i2c read error (first attempt)");
    if (!trafficSeen)
        flxLog_W("GNSS::isConnected no traffic seen (first attempt)");

    // If the GNSS is silent (e.g. NMEA disabled and NAV-PVT not periodic - Flux Issue #104),
    // manually set the NAV-PVT to periodic (rate 1) and check again for traffic
    uint8_t periodicNAVPVT[11] = { 0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0x01, 0x07, 0x01, 0x13, 0x51 };
    i2cOK = i2cDriver.write(address, periodicNAVPVT, 11); // Will write to address 0xFF

    // Wait for up to 2 seconds for more bytes to be added (ACK plus first NAV-PVT)
    startTime = millis();
    while(i2cOK && (!trafficSeen) && (millis() < (startTime + 2000)))
    {
        delay(100); // Don't pound the bus
        i2cOK = i2cDriver.readRegister16(address, 0xFD, &bufferWaiting, false); // Big Endian
        if (i2cOK)
            trafficSeen = (bufferWaiting != firstBufferWaiting); // Has traffic been seen?
    }

    if (i2cOK && trafficSeen)
        return true; // Return now if traffic has been seen

    if (!i2cOK)
        flxLog_E("GNSS::isConnected i2c read error (second attempt)");
    if (!trafficSeen)
        flxLog_W("GNSS::isConnected no traffic seen (second attempt)");

    // If the GNSS has been powered on for some time, the buffer could be full
    // Try to read some data from the buffer and see if the count changes
    if (bufferWaiting >= 8)
    {
        uint8_t buffer[8];
        i2cOK = i2cDriver.receiveResponse(address, buffer, 8) == 8; // Will read from address 0xFF
        i2cOK &= i2cDriver.readRegister16(address, 0xFD, &bufferWaiting, false); // Big Endian
        trafficSeen = (bufferWaiting != firstBufferWaiting);
    }

    if (!i2cOK)
        flxLog_E("GNSS::isConnected i2c read error (third attempt)");
    if (!trafficSeen)
        flxLog_W("GNSS::isConnected no traffic seen (third attempt)");

    return (i2cOK && trafficSeen);
}

//----------------------------------------------------------------------------------------------------------
// onInitialize()
//
// Called during the startup/initialization of the driver (after the constructor is called).
//
// Place to initialized the underlying device library/driver
//
bool flxDevGNSS::onInitialize(TwoWire &wirePort)
{

    bool result = SFE_UBLOX_GNSS::begin(wirePort);
    if (result)
    {
        SFE_UBLOX_GNSS::setI2COutput(COM_TYPE_UBX); // Set the I2C port to output UBX only (turn off NMEA noise)
        SFE_UBLOX_GNSS::setAutoPVT(true);           // Enable PVT at the navigation rate
        SFE_UBLOX_GNSS::saveConfigSelective(VAL_CFG_SUBSEC_IOPORT | VAL_CFG_SUBSEC_MSGCONF); // Save the port and message settings to flash and BBR
        delay(1100);
        SFE_UBLOX_GNSS::getPVT(); // Ensure we get fresh data
    }
    return result;
}

// GETTER methods for output params
uint flxDevGNSS::read_year() { return SFE_UBLOX_GNSS::getYear(); }
uint flxDevGNSS::read_month() { return SFE_UBLOX_GNSS::getMonth(); }
uint flxDevGNSS::read_day() { return SFE_UBLOX_GNSS::getDay(); }
uint flxDevGNSS::read_hour() { return SFE_UBLOX_GNSS::getHour(); }
uint flxDevGNSS::read_min() { return SFE_UBLOX_GNSS::getMinute(); }
uint flxDevGNSS::read_sec() { return SFE_UBLOX_GNSS::getSecond(); }
double flxDevGNSS::read_latitude() { return (((double)SFE_UBLOX_GNSS::getLatitude()) / 10000000); }
double flxDevGNSS::read_longitude() { return (((double)SFE_UBLOX_GNSS::getLongitude()) / 10000000); }
double flxDevGNSS::read_altitude() { return (((double)SFE_UBLOX_GNSS::getAltitude()) / 1000); }
double flxDevGNSS::read_altitude_msl() { return (((double)SFE_UBLOX_GNSS::getAltitudeMSL()) / 1000); }
uint flxDevGNSS::read_siv() { return SFE_UBLOX_GNSS::getSIV(); }
uint flxDevGNSS::read_fix() { return SFE_UBLOX_GNSS::getFixType(); }
uint flxDevGNSS::read_carrier_soln() { return SFE_UBLOX_GNSS::getCarrierSolutionType(); }
float flxDevGNSS::read_ground_speed() { return (((float)SFE_UBLOX_GNSS::getGroundSpeed()) / 1000); }
float flxDevGNSS::read_heading() { return (((float)SFE_UBLOX_GNSS::getHeading()) / 100000); }
float flxDevGNSS::read_horiz_acc() { return (((float)SFE_UBLOX_GNSS::getHorizontalAccEst()) / 1000); }
float flxDevGNSS::read_vert_acc() { return (((float)SFE_UBLOX_GNSS::getVerticalAccEst()) / 1000); }
float flxDevGNSS::read_pdop() { return (((float)SFE_UBLOX_GNSS::getPDOP()) / 100); }
uint flxDevGNSS::read_tow() { return SFE_UBLOX_GNSS::getTimeOfWeek(); }

std::string flxDevGNSS::read_iso8601()
{
    uint y = SFE_UBLOX_GNSS::getYear();
    uint M = SFE_UBLOX_GNSS::getMonth();
    uint d = SFE_UBLOX_GNSS::getDay();
    uint h = SFE_UBLOX_GNSS::getHour();
    uint m = SFE_UBLOX_GNSS::getMinute();
    uint s = SFE_UBLOX_GNSS::getSecond();

    char szBuffer[32] = {'\0'};
    snprintf(szBuffer, sizeof(szBuffer), "%04d-%02d-%02dT%02d:%02d:%02dZ", y, M, d, h, m, s);

    std::string theString = szBuffer;

    return theString;
}

std::string flxDevGNSS::read_yyyy_mm_dd()
{
    uint y = SFE_UBLOX_GNSS::getYear();
    uint M = SFE_UBLOX_GNSS::getMonth();
    uint d = SFE_UBLOX_GNSS::getDay();

    char szBuffer[24] = {'\0'};
    snprintf(szBuffer, sizeof(szBuffer), "%04d/%02d/%02d", y, M, d);

    std::string theString = szBuffer;

    return theString;
}

std::string flxDevGNSS::read_yyyy_dd_mm()
{
    uint y = SFE_UBLOX_GNSS::getYear();
    uint M = SFE_UBLOX_GNSS::getMonth();
    uint d = SFE_UBLOX_GNSS::getDay();

    char szBuffer[24] = {'\0'};
    snprintf(szBuffer, sizeof(szBuffer), "%04d/%02d/%02d", y, d, M);

    std::string theString = szBuffer;

    return theString;
}

std::string flxDevGNSS::read_dd_mm_yyyy()
{
    uint y = SFE_UBLOX_GNSS::getYear();
    uint M = SFE_UBLOX_GNSS::getMonth();
    uint d = SFE_UBLOX_GNSS::getDay();

    char szBuffer[24] = {'\0'};
    snprintf(szBuffer, sizeof(szBuffer), "%02d/%02d/%04d", d, M, y);

    std::string theString = szBuffer;

    return theString;
}

std::string flxDevGNSS::read_hh_mm_ss()
{
    uint h = SFE_UBLOX_GNSS::getHour();
    uint m = SFE_UBLOX_GNSS::getMinute();
    uint s = SFE_UBLOX_GNSS::getSecond();

    char szBuffer[24] = {'\0'};
    snprintf(szBuffer, sizeof(szBuffer), "%02d:%02d:%02d", h, m, s);

    std::string theString = szBuffer;

    return theString;
}

std::string flxDevGNSS::read_fix_string()
{
    uint fix = SFE_UBLOX_GNSS::getFixType();

    char szBuffer[24] = {'\0'};

    switch (fix)
    {
    case 0:
        snprintf(szBuffer, sizeof(szBuffer), "none");
        break;
    case 1:
        snprintf(szBuffer, sizeof(szBuffer), "dead_reckoning");
        break;
    case 2:
        snprintf(szBuffer, sizeof(szBuffer), "2D");
        break;
    case 3:
        snprintf(szBuffer, sizeof(szBuffer), "3D");
        break;
    case 4:
        snprintf(szBuffer, sizeof(szBuffer), "GNSS_+_dead_reckoning");
        break;
    case 5:
        snprintf(szBuffer, sizeof(szBuffer), "time_only");
        break;
    default:
        snprintf(szBuffer, sizeof(szBuffer), "unknown");
        break;
    }

    std::string theString = szBuffer;

    return theString;
}

std::string flxDevGNSS::read_carrier_soln_string()
{
    uint carrSoln = SFE_UBLOX_GNSS::getCarrierSolutionType();

    char szBuffer[24] = {'\0'};

    switch (carrSoln)
    {
    case 0:
        snprintf(szBuffer, sizeof(szBuffer), "none");
        break;
    case 1:
        snprintf(szBuffer, sizeof(szBuffer), "floating");
        break;
    case 2:
        snprintf(szBuffer, sizeof(szBuffer), "fixed");
        break;
    default:
        snprintf(szBuffer, sizeof(szBuffer), "unknown");
        break;
    }

    std::string theString = szBuffer;

    return theString;
}

// methods for read-write properties
uint flxDevGNSS::get_measurement_rate() { return SFE_UBLOX_GNSS::getMeasurementRate(); }
void flxDevGNSS::set_measurement_rate(uint rate) { SFE_UBLOX_GNSS::setMeasurementRate(rate); }

// methods for in parameters
void flxDevGNSS::factory_default()
{
    SFE_UBLOX_GNSS::factoryDefault();
    delay(5000);
    SFE_UBLOX_GNSS::setI2COutput(COM_TYPE_UBX);
    SFE_UBLOX_GNSS::setAutoPVT(true);
    SFE_UBLOX_GNSS::saveConfigSelective(VAL_CFG_SUBSEC_IOPORT | VAL_CFG_SUBSEC_MSGCONF);
}

//----------------------------------------------------------------------------------------------------------
// Loop

bool flxDevGNSS::loop(void)
{
    SFE_UBLOX_GNSS::checkUblox();
    return false;
}
