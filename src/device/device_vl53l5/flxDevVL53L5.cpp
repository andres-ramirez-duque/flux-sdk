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
 *  flxDevVL53L5.h
 *
 *  Spark Device object for the VL53L5 device.
 *
 *
 *
 */
#include "Arduino.h"

#include "flxDevVL53L5.h"

// Define our class static variables - allocs storage for them

#define kVL53L5AddressDefault 0x29 // Unshifted

uint8_t flxDevVL53L5::defaultDeviceAddress[] = {kVL53L5AddressDefault, kSparkDeviceAddressNull};

//----------------------------------------------------------------------------------------------------------
// Register this class with the system, enabling this driver during system
// initialization and device discovery.

flxRegisterDevice(flxDevVL53L5);

//----------------------------------------------------------------------------------------------------------
// Constructor
//
// Object constructor. Performs initialization of device values, including device identifiers (name, I2C address),
// and managed properties.

flxDevVL53L5::flxDevVL53L5()
{

    // Setup unique identifiers for this device and basic device object systems
    setName(getDeviceName());
    setDescription("VL53L5 Distance Sensor");

    // Register parameters
    flxRegister(distance, "Distance (mm)", "The measured distances in mm");

    // Register read-write properties
    flxRegister(integrationTime, "Integration Time", "The selected integration time in milliseconds");
    flxRegister(sharpenerPercent, "Sharpener Percent", "The selected sharpener value in percent");
    flxRegister(targetOrder, "Target Order", "The selected targeting mode");
}

//----------------------------------------------------------------------------------------------------------
// Static method used to determine if devices is connected before creating this object (if creating dynamically)
bool flxDevVL53L5::isConnected(flxBusI2C &i2cDriver, uint8_t address)
{
    // For speed, ping the device address first
    if (!i2cDriver.ping(address))
        return false;

    uint8_t writePrep[3] = {0x7f, 0xff, 0x00};
    bool couldBeVL53L = i2cDriver.write(address, writePrep, 3);
    uint8_t devIdPrep[2] = {0x00, 0x00};
    couldBeVL53L &= i2cDriver.write(address, devIdPrep, 2);
    uint8_t devID = 0;
    couldBeVL53L &= i2cDriver.receiveResponse(address, &devID, 1) == 1;
    uint8_t revIdPrep[2] = {0x00, 0x01};
    couldBeVL53L &= i2cDriver.write(address, revIdPrep, 2);
    uint8_t revID = 0;
    couldBeVL53L &= i2cDriver.receiveResponse(address, &revID, 1) == 1;
    uint8_t postPrep[3] = {0x7f, 0xff, 0x02};
    couldBeVL53L &= i2cDriver.write(address, postPrep, 3);

    couldBeVL53L &= ((devID == 0xF0) && (revID == 0x02));

    return (couldBeVL53L);
}

//----------------------------------------------------------------------------------------------------------
// onInitialize()
//
// Called during the startup/initialization of the driver (after the constructor is called).
//
// Place to initialized the underlying device library/driver
//
bool flxDevVL53L5::onInitialize(TwoWire &wirePort)
{
    flxLog_W("VL53L5 is being initialized. This could take 10 seconds...");

    bool status = SparkFun_VL53L5CX::begin(flxDevice::address(), wirePort);
    if (status)
    {
        SparkFun_VL53L5CX::setResolution(8 * 8);
        SparkFun_VL53L5CX::startRanging();
    }
    return status;
}

// GETTER methods for output params
bool flxDevVL53L5::read_distance(flxDataArrayInt16 *distances)
{
    static int16_t theDistances[64] = {-999};

    VL53L5CX_ResultsData measurementData; // Result data class structure, 1356 byes of RAM

    bool result = SparkFun_VL53L5CX::isDataReady();

    if (result)
    {
        result &= SparkFun_VL53L5CX::getRangingData(&measurementData);

        int i = 0;
        for (int y = 0; y < 64; y += 8)
            for (int x = 7; x >= 0; x--)
                theDistances[i++] = measurementData.distance_mm[x + y];

        distances->set(theDistances, 8, 8, true); // don't copy
    }

    return result;
}

// methods for read-write properties
uint32_t flxDevVL53L5::get_integration_time()
{
    if (isInitialized())
        _integrationTime = SparkFun_VL53L5CX::getIntegrationTime();
    return _integrationTime;
}

void flxDevVL53L5::set_integration_time(uint32_t intTime)
{
    _integrationTime = intTime;
    if (isInitialized())
    {
        SparkFun_VL53L5CX::stopRanging();
        SparkFun_VL53L5CX::setIntegrationTime(intTime);
        SparkFun_VL53L5CX::startRanging();
    }
}

uint8_t flxDevVL53L5::get_sharpener_percent()
{
    if (isInitialized())
        _sharpenerPercent = SparkFun_VL53L5CX::getSharpenerPercent();
    return _sharpenerPercent;
}

void flxDevVL53L5::set_sharpener_percent(uint8_t percent)
{
    _sharpenerPercent = percent;

    if (isInitialized())
    {
        SparkFun_VL53L5CX::stopRanging();
        SparkFun_VL53L5CX::setSharpenerPercent(percent);
        SparkFun_VL53L5CX::startRanging();
    }
}

uint8_t flxDevVL53L5::get_target_order()
{
    if (isInitialized())
        _targetOrder = (uint8_t)SparkFun_VL53L5CX::getTargetOrder();
    return _targetOrder;
}

void flxDevVL53L5::set_target_order(uint8_t order)
{
    _targetOrder = order;
    if (isInitialized())
    {
        SparkFun_VL53L5CX::stopRanging();
        SparkFun_VL53L5CX::setTargetOrder((SF_VL53L5CX_TARGET_ORDER)order);
        SparkFun_VL53L5CX::startRanging();
    }
}
