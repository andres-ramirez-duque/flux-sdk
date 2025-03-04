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

const uint8_t kCalibrationIterations = 5;

const uint16_t kCachedValueDeltaTicks = 1000;

//----------------------------------------------------------------------------------------------------------
// Constructor
//
// Object constructor. Performs initialization of device values, including device identifiers (name, I2C address),
// and managed properties.

flxDevSoilMoisture::flxDevSoilMoisture()
    : _pinVCC{kNoPinSet}, _pinSensor{kNoPinSet}, _isEnabled{false}, _lowCalVal{0}, _highCalVal{900}, _lastValueTick{0}
{

    // Setup unique identifiers for this device and basic device object systems
    setName(getDeviceName());
    setDescription("The SparkFun Soil Moisture Sensor");

    // Register properties
    flxRegister(isEnabled, "Enable this sensor", "When true, this sensor is enabled");
    flxRegister(vccPin, "VCC Pin", "The power (VCC) GPIO pin connected to the soil sensor. 0 = disabled");
    flxRegister(sensorPin, "Sensor Pin", "The sensor GPIO pin connected to the soil sensor. 0 = disabled");
    flxRegister(calibrationDry, "Calibration Dry Value", "The calibrated value for dry (0% moisture)");
    flxRegister(calibrationWet, "Calibration Wet Value", "The calibrated value for wet (100% moisture)");

    // Functions
    flxRegister(calibrateLowValue, "Calibrate Low (dry) Value", "Set the 0% moist (dry) value of the sensor");
    flxRegister(calibrateHighValue, "Calibrate High (wet) Value", "Set the 100% moist value of the sensor");

    // Register parameters
    flxRegister(moistureValue, "Moisture Sensor Value", "A value of dry (0) to wet", kParamValueSoilMoistureRaw);
    flxRegister(moisturePercent, "Percent Moisture", "Value between 0.0% and 100.0%", kParamValueSoilMoisturePercent_F);
}

//----------------------------------------------------------------------------------------------------------
// onInitialize()
//
// Called during the startup/initialization of the driver (after the constructor is called).
//
// Place to initialize the underlying device library/driver
//
//-----------------------------------------------------------------------
bool flxDevSoilMoisture::onInitialize(void)
{
    if (isInitialized())
        return true;

    return setupSensor();
}
//-----------------------------------------------------------------------
bool flxDevSoilMoisture::setupSensor(void)
{

    // Pins define yet?
    if (_pinVCC == kNoPinSet || _pinSensor == kNoPinSet)
        return false;

    // setup our power pin - enable output, set to low
    pinMode(_pinVCC, OUTPUT);
    digitalWrite(_pinVCC, LOW);

    // this is cal
    setIsInitialized(true);

    return true;
}
//-----------------------------------------------------------------------
//  Properties
//-----------------------------------------------------------------------

bool flxDevSoilMoisture::get_is_enabled(void)
{
    return _isEnabled;
}
void flxDevSoilMoisture::set_is_enabled(bool enable)
{
    if (enable == _isEnabled)
        return;

    _isEnabled = enable;
    // are we turning this on?
    if (enable)
        setupSensor();
}

uint8_t flxDevSoilMoisture::get_vcc_pin(void)
{
    return _pinVCC;
}
void flxDevSoilMoisture::set_vcc_pin(uint8_t newPin)
{

    if (_pinVCC == newPin)
        return;

    _pinVCC = newPin;

    // If this is a no pin set value, disable sensor
    if (newPin == kNoPinSet)
        set_is_enabled(false);
    else
        setupSensor(); // new pin, do the setup.
}

uint8_t flxDevSoilMoisture::get_sensor_pin(void)
{
    return _pinSensor;
}
void flxDevSoilMoisture::set_sensor_pin(uint8_t newPin)
{

    if (_pinSensor == newPin)
        return;

    _pinSensor = newPin;
    setAddress(_pinSensor);

    // If this is a no pin set value, disable sensor
    if (newPin == kNoPinSet)
        set_is_enabled(false);
}

//-----------------------------------------------------------------------
// GETTER methods for output params
//-----------------------------------------------------------------------
uint16_t flxDevSoilMoisture::read_moisture_value()
{
    // This returns the RAW sensor value
    if (!isInitialized() || !_isEnabled)
        return 0;

    // do we have a cached value?
    if ((millis() - _lastValueTick) > kCachedValueDeltaTicks)
    {
        // get the value from the sensor
        // enable power
        digitalWrite(_pinVCC, HIGH);
        delay(30);
        _lastValue = analogRead(_pinSensor);
        _lastValueTick = millis();
        // power off
        digitalWrite(_pinVCC, LOW);
    }
    return _lastValue;
}

//-----------------------------------------------------------------------
float flxDevSoilMoisture::read_moisture_percent()
{
    float sensorValue = (float)read_moisture_value();

    return (100. / (_highCalVal - _lowCalVal)) * (sensorValue - (float)_lowCalVal);
}

//-----------------------------------------------------------------------
// calibration
//-----------------------------------------------------------------------
void flxDevSoilMoisture::calibrate_low_value(void)
{
    // check if sensor is up and setup
    if (!_isEnabled || !isInitialized())
    {
        flxLog_W("%s: Sensor not setup and enabled. Unable to continue", name());
        return;
    }
    uint32_t valueSum = 0;

    flxLog_N_(F("Calibrating sensor dry value.."));

    for (int i = 0; i < kCalibrationIterations; i++)
    {
        valueSum += read_moisture_value();
        delay(kCachedValueDeltaTicks + 1);
        flxLog_N_(".");
    }

    _lowCalVal = valueSum / kCalibrationIterations;
    flxLog_N(F("Calibration complete. Dry value is: %d"), _lowCalVal);

    // so this value is saved
    this->setIsDirty();
}
//-----------------------------------------------------------------------
void flxDevSoilMoisture::calibrate_high_value(void)
{
    // check if sensor is up and setup
    if (!_isEnabled || !isInitialized())
    {
        flxLog_W("%s: Sensor not setup and enabled. Unable to continue", name());
        return;
    }
    uint32_t valueSum = 0;

    flxLog_N_(F("Calibrating sensor 100% wet value.."));
    for (int i = 0; i < kCalibrationIterations; i++)
    {
        valueSum += read_moisture_value();
        delay(kCachedValueDeltaTicks + 1);
        flxLog_N_(".");
    }
    _highCalVal = valueSum / kCalibrationIterations;
    flxLog_N(F("Calibration complete. 100%c Web value is: %d"), '%', _highCalVal);
    // so this value is saved
    this->setIsDirty();
}