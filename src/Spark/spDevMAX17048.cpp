/*
 *
 *  spDevMAX17048.cpp
 *
 *  Spark Device object for the MAX17048 device.
 *
 *
 */

#include "Arduino.h"

#include "spDevMAX17048.h"

uint8_t spDevMAX17048::defaultDeviceAddress[] = {MAX1704x_ADDRESS, kSparkDeviceAddressNull};

// Register this class with the system - this enables the *auto load* of this device
spRegisterDevice(spDevMAX17048);

spDevMAX17048::spDevMAX17048()
{

    spSetupDeviceIdent(getDeviceName());
    setDescription("MAX17048 LiPo Battery Fuel Gauge");

    // Register output params
    spRegister(voltageV, "Voltage (V)", "Battery voltage (Volts)");
    spRegister(stateOfCharge, "State Of Charge (%)", "Battery state of charge (%)");
    spRegister(changeRate, "Change Rate (%/hr)", "Battery charge change rate (%/hr)");
}

// Function to encapsulate the ops needed to get values from the sensor.
float spDevMAX17048::read_voltage()
{
    return SFE_MAX1704X::getVoltage();
}
float spDevMAX17048::read_state_of_charge()
{
    return SFE_MAX1704X::getSOC();
}
float spDevMAX17048::read_change_rate()
{
    return SFE_MAX1704X::getChangeRate();
}

// Static method used to determine if this device is connected

bool spDevMAX17048::isConnected(spDevI2C &i2cDriver, uint8_t address)
{
    // For speed, ping the device address first
    if (!i2cDriver.ping(address))
        return false;

    // Updated to resolve issue #4 Dec 27th 2021
    // Also avoid using the standard "if device answers on _deviceAddress" test
    //(https://github.com/sparkfun/Arduino_Apollo3/issues/400#issuecomment-992631994)
    bool success = false;
    uint8_t retries = 3;
    uint16_t version = 0;

    while ((success == false) && (retries > 0))
    {
        if (i2cDriver.readRegister16(address, MAX17043_VERSION, &version, false)) // Attempt to read the version register
        {
            success = true;
        }
        else
        {
            retries--;
            spLog_W("SFE_MAX1704X::isConnected: retrying...");
            delay(50);
        }
    }

    if (!success) // Return now if the version could not be read
    {
        spLog_E("SFE_MAX1704X::isConnected: failed to detect IC!");
        return (success);
    }

    // Extra test - but only for MAX17048/9 - see issue #4
    {
        // Get version should return 0x001_
        // Not a great test but something
        // Supported on 48/49
        if ((version & (1 << 4)) == 0)
            success = false;
    }

    return (success);
}

//----------------------------------------------------------------------------------------------------------
// onInitialize()
//
// Called during the startup/initialization of the driver (after the constructor is called).
//
// Place to initialized the underlying device library/driver
//
bool spDevMAX17048::onInitialize(TwoWire &wirePort)
{
    // set the underlying drivers address to the one determined during
    // device construction
    SFE_MAX1704X::setDevice(MAX1704X_MAX17048);
    bool result = SFE_MAX1704X::begin(wirePort);

    if (!result)
        spLog_E("MAX17048 - begin failed");

    return result;
}

