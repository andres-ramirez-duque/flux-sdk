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
 *  flxDevVCNL4040.h
 *
 *  Spark Device object for the VCNL4040 device.
 *
 *
 *
 */
#include "Arduino.h"

#include "flxDevVCNL4040.h"

// Define our class static variables - allocs storage for them

#define kVCNL4040AddressDefault 0x60 // VCNL4040_ADDR

uint8_t flxDevVCNL4040::defaultDeviceAddress[] = {kVCNL4040AddressDefault, kSparkDeviceAddressNull};

//----------------------------------------------------------------------------------------------------------
// Register this class with the system, enabling this driver during system
// initialization and device discovery.

flxRegisterDevice(flxDevVCNL4040);

//----------------------------------------------------------------------------------------------------------
// Constructor
//
// Object constructor. Performs initialization of device values, including device identifiers (name, I2C address),
// and managed properties.

flxDevVCNL4040::flxDevVCNL4040()
{

    // Setup unique identifiers for this device and basic device object systems
    setName(getDeviceName());
    setDescription("VCNL4040 Proximity Sensor");

    // Register parameters
    flxRegister(proximity, "Proximity", "Proximity : high values indicate close proximity", kParamValueProximity);
    flxRegister(lux, "Lux", "The light level in lux", kParamValueLUX_U16);

    // Register read-write properties
    flxRegister(ledCurrent, "LED Current (mA)", "The LED current in milliamps)");
    flxRegister(irDutyCycle, "IR Duty Cycle", "The InfraRed duty cycle");
    flxRegister(proximityIntegrationTime, "Proximity Integration Time", "The proximity measurement integration time");
    flxRegister(proximityResolution, "Proximity Resolution", "The proximity measurement resolution");
    flxRegister(ambientIntegrationTime, "Ambient Integration Time (ms)",
                "The ambient light level integration time in milliseconds");
}

//----------------------------------------------------------------------------------------------------------
// Static method used to determine if devices is connected before creating this object (if creating dynamically)
bool flxDevVCNL4040::isConnected(flxBusI2C &i2cDriver, uint8_t address)
{
    // For speed, ping the device address first
    if (!i2cDriver.ping(address))
        return false;

    uint16_t idReg;                                                           // VCNL4040_ID
    bool couldBe4040 = i2cDriver.readRegister16(address, 0x0C, &idReg, true); // Little Endian
    couldBe4040 &= (idReg == 0x0186);

    return (couldBe4040);
}

//----------------------------------------------------------------------------------------------------------
// onInitialize()
//
// Called during the startup/initialization of the driver (after the constructor is called).
//
// Place to initialized the underlying device library/driver
//
bool flxDevVCNL4040::onInitialize(TwoWire &wirePort)
{

    bool status = VCNL4040::begin(wirePort);
    if (status)
    {
        VCNL4040::powerOnAmbient(); // Turn on ambient sensing
        VCNL4040::setLEDCurrent(_ledCurrent);
        VCNL4040::setIRDutyCycle(_irDutyCycle);
        VCNL4040::setProxIntegrationTime(_proxIntTime);
        VCNL4040::setProxResolution(_proxRes);
        VCNL4040::setAmbientIntegrationTime(_ambIntTime);
    }
    return status;
}

// GETTER methods for output params
uint16_t flxDevVCNL4040::read_proximity()
{
    return VCNL4040::getProximity();
}

uint16_t flxDevVCNL4040::read_lux()
{
    return VCNL4040::getAmbient();
}

// methods for read-write properties
uint8_t flxDevVCNL4040::get_LED_current()
{
    return _ledCurrent;
}

void flxDevVCNL4040::set_LED_current(uint8_t current)
{
    _ledCurrent = current;
    if (isInitialized())
        VCNL4040::setLEDCurrent(current);
}

uint16_t flxDevVCNL4040::get_IR_duty_cycle()
{
    return _irDutyCycle;
}

void flxDevVCNL4040::set_IR_duty_cycle(uint16_t duty)
{
    _irDutyCycle = duty;
    if (isInitialized())
        VCNL4040::setIRDutyCycle(duty);
}

uint8_t flxDevVCNL4040::get_proximity_integration_time()
{
    return _proxIntTime;
}

void flxDevVCNL4040::set_proximity_integration_time(uint8_t intTime)
{
    _proxIntTime = intTime;
    if (isInitialized())
        VCNL4040::setProxIntegrationTime(intTime);
}

uint8_t flxDevVCNL4040::get_proximity_resolution()
{
    return _proxRes;
}

void flxDevVCNL4040::set_proximity_resolution(uint8_t res)
{
    _proxRes = res;
    if (isInitialized())
        VCNL4040::setProxResolution(res);
}

uint16_t flxDevVCNL4040::get_ambient_integration_time()
{
    return _ambIntTime;
}

void flxDevVCNL4040::set_ambient_integration_time(uint16_t intTime)
{
    _ambIntTime = intTime;
    if (isInitialized())
        VCNL4040::setAmbientIntegrationTime(intTime);
}
