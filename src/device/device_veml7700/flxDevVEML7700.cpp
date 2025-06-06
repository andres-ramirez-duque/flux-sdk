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
 *  flxDevVEML7700.h
 *
 *  Spark Device object for the VEML7700 device.
 *
 *
 *
 */
#include "Arduino.h"

#include "flxDevVEML7700.h"

// Define our class static variables - allocs storage for them

#define kVEML7700AddressDefault VEML7700_I2C_ADDRESS

uint8_t flxDevVEML7700::defaultDeviceAddress[] = {kVEML7700AddressDefault, kSparkDeviceAddressNull};

//----------------------------------------------------------------------------------------------------------
// Register this class with the system, enabling this driver during system
// initialization and device discovery.

flxRegisterDevice(flxDevVEML7700);

//----------------------------------------------------------------------------------------------------------
// Constructor
//
// Object constructor. Performs initialization of device values, including device identifiers (name, I2C address),
// and managed properties.

flxDevVEML7700::flxDevVEML7700()
{

    // Setup unique identifiers for this device and basic device object systems
    setName(getDeviceName());
    setDescription("VEML7700 Ambient Light Sensor");

    // Register parameters
    flxRegister(ambientLight, "Ambient Light Level", "The ambient light level", kParamValueAmbientLight);
    flxRegister(whiteLevel, "White Level", "The measured light white level", kParamValueWhiteLight);
    flxRegister(lux, "Lux", "The light level in lux", kParamValueLUX_F);

    // Register read-write properties
    flxRegister(integrationTime, "Integration Time (ms)", "The measurement integration time in milliseconds");
    flxRegister(sensitivity, "Sensitivity", "The measurement sensitivity");
    flxRegister(persistence, "Persistence", "The measurement persistence");
}

//----------------------------------------------------------------------------------------------------------
// Static method used to determine if devices is connected before creating this object (if creating dynamically)
bool flxDevVEML7700::isConnected(flxBusI2C &i2cDriver, uint8_t address)
{
    // For speed, ping the device address first
    if (!i2cDriver.ping(address))
        return false;

    // The VEML7700 does not have an ID register
    // The best we can do is read the config register and check that bits 15-13, 10 and 3-2 are zero
    // But the VEML6075 will also pass this test...
    uint16_t configReg;
    bool couldBe7700 = i2cDriver.readRegister16(address, 0, &configReg, true); // Little Endian
    couldBe7700 &= ((configReg & 0b1110010000001100) == 0);

    // Check if this is a VEML6075
    uint16_t veml6075DeviceID = 0;
    bool identify6075 = i2cDriver.readRegister16(address, 0x0C, &veml6075DeviceID, true); // Little Endian
    identify6075 &= (veml6075DeviceID == 0x0026);                                         // VEML6075_DEVICE_ID

    return ((couldBe7700) && (!identify6075));
}

//----------------------------------------------------------------------------------------------------------
// onInitialize()
//
// Called during the startup/initialization of the driver (after the constructor is called).
//
// Place to initialized the underlying device library/driver
//
bool flxDevVEML7700::onInitialize(TwoWire &wirePort)
{
    return VEML7700::begin(wirePort);
}

// GETTER methods for output params
uint32_t flxDevVEML7700::read_ambient_light()
{
    return VEML7700::getAmbientLight();
}

uint32_t flxDevVEML7700::read_white_level()
{
    return VEML7700::getWhiteLevel();
}

float flxDevVEML7700::read_lux()
{
    return VEML7700::getLux();
}

// methods for read-write properties
uint8_t flxDevVEML7700::get_integration_time()
{
    if (isInitialized())
        _integrationTime = VEML7700::getIntegrationTime();
    return _integrationTime;
}

void flxDevVEML7700::set_integration_time(uint8_t intTime)
{
    _integrationTime = intTime;
    if (isInitialized())
        VEML7700::setIntegrationTime((VEML7700_integration_time_t)intTime);
}

uint8_t flxDevVEML7700::get_sensitivity()
{
    if (isInitialized())
        _sensitivity = VEML7700::getSensitivityMode();
    return _sensitivity;
}

void flxDevVEML7700::set_sensitivity(uint8_t sens)
{
    _sensitivity = sens;
    if (isInitialized())
        VEML7700::setSensitivityMode((VEML7700_sensitivity_mode_t)sens);
}

uint8_t flxDevVEML7700::get_persistence()
{
    if (isInitialized())
        _persistence = VEML7700::getPersistenceProtect();
    return _persistence;
}

void flxDevVEML7700::set_persistence(uint8_t pers)
{
    _persistence = pers;
    if (isInitialized())
        VEML7700::setPersistenceProtect((VEML7700_persistence_protect_t)pers);
}
