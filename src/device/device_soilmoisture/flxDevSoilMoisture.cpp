/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2025, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

/*
 *
 *  flxDevSoilMoisture.cpp
 *
 *  Device object for the SparkFun Soil Moisture sensor.
 *
 *
 *
 */
#include "Arduino.h"

#include "flxDevSoilMoisture.h"

// Define our class static variables - allocs storage for them

uint8_t flxDevSoilMoisture::defaultDeviceAddress[] = {SF_SOIL_MOISTURE_DEFAULT_I2C_ADDRESS, kSparkDeviceAddressNull};

//----------------------------------------------------------------------------------------------------------
// Register this class with the system, enabling this driver during system
// initialization and device discovery.

flxRegisterDevice(flxDevSoilMoisture);

//----------------------------------------------------------------------------------------------------------
// Constructor
//
// Object constructor. Performs initialization of device values, including device identifiers (name, I2C address),
// and managed properties.

flxDevSoilMoisture::flxDevSoilMoisture()
{

    // Setup unique identifiers for this device and basic device object systems
    setName(getDeviceName());
    setDescription("The SparkFun Soil Moisture Sensor");

    // Register parameters
    flxRegister(moistureValue, "Moisture Sensor Value", "A value of 0 (wet) to 1023 (dry)", kParamValueSoilMoistureRaw);
    flxRegister(moisturePercent, "Percent Moisture", "Value between 0.0% and 100.0%", kParamValueSoilMoisturePercent_F);
}

//----------------------------------------------------------------------------------------------------------
// Static method used to determine if devices is connected before creating this object (if creating dynamically)
bool flxDevSoilMoisture::isConnected(flxBusI2C &i2cDriver, uint8_t address)
{
    // For speed, ping the device address first
    return i2cDriver.ping(address);
}
//----------------------------------------------------------------------------------------------------------
// onInitialize()
//
// Called during the startup/initialization of the driver (after the constructor is called).
//
// Place to initialized the underlying device library/driver
//
bool flxDevSoilMoisture::onInitialize(TwoWire &wirePort)
{

    if (SparkFunSoilMoistureSensor::begin(flxDevice::address(), wirePort) == false)
    {
        flxLog_E("%s: failed to initialize", getDeviceName());
        return false;
    }

    return true;
}

// GETTER methods for output params
uint16_t flxDevSoilMoisture::read_moisture_value()
{

    return SparkFunSoilMoistureSensor::readMoistureValue();
}

float flxDevSoilMoisture::read_moisture_percent()
{
    return SparkFunSoilMoistureSensor::readMoisturePercentage();
}
