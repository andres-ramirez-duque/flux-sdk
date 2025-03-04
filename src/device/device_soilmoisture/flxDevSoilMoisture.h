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
 *  flxDevSoilMoisture.h
 *
 *  Device object for the SparkFun Soil Moisture sensor.
 *
 *
 *
 */

#pragma once

#include "Arduino.h"

#include "flxDevice.h"

// What is the name used to ID this device?
#define kSoilMoistureDeviceName "Soil Moisture Sensor"
//----------------------------------------------------------------------------------------------------------
// Define our class - This is a simple GPIO driven device
class flxDevSoilMoisture : public flxDeviceGPIOType<flxDevSoilMoisture>
{

  public:
    flxDevSoilMoisture();

    flxDevSoilMoisture(uint8_t pinVCC, uint8_t pinSensor) : _pinVCC{pinVCC}, _pinSensor{pinSensor}
    {
    }

    static const char *getDeviceName()
    {
        return kSoilMoistureDeviceName;
    };

    // Method called to initialize the class
    bool onInitialize(void);

  private:
    // consts
    static constexpr uint8_t kNoPinSet = 0;

    // Setup Sensor

    bool setupSensor(void);

    // props
    // is enabled?
    bool get_is_enabled(void);
    void set_is_enabled(bool);

    uint8_t get_vcc_pin(void);
    void set_vcc_pin(uint8_t);

    uint8_t get_sensor_pin(void);
    void set_sensor_pin(uint8_t);

    uint16_t get_cal_low(void)
    {
        return _lowCalVal;
    }
    void set_cal_low(uint16_t val)
    {
        _lowCalVal = val;
    }
    uint16_t get_cal_high(void)
    {
        return _highCalVal;
    }
    void set_cal_high(uint16_t val)
    {
        _highCalVal = val;
    }

    // methods used to get values for our output parameters
    uint16_t read_moisture_value();
    float read_moisture_percent();

    void calibrate_low_value(void);
    void calibrate_high_value(void);

    uint8_t _pinVCC;
    uint8_t _pinSensor;

    bool _isEnabled;

    uint16_t _lowCalVal;
    uint16_t _highCalVal;

    // cache the last value read - b/c value can change between read in the same observation
    uint16_t _lastValue;
    uint32_t _lastValueTick;

  public:
    // properties
    flxPropertyRWBool<flxDevSoilMoisture, &flxDevSoilMoisture::get_is_enabled, &flxDevSoilMoisture::set_is_enabled>
        isEnabled = {false};

    flxPropertyRWUInt8<flxDevSoilMoisture, &flxDevSoilMoisture::get_vcc_pin, &flxDevSoilMoisture::set_vcc_pin> vccPin;
    flxPropertyRWUInt8<flxDevSoilMoisture, &flxDevSoilMoisture::get_sensor_pin, &flxDevSoilMoisture::set_sensor_pin>
        sensorPin;

    // cal values
    flxPropertyRWUInt16<flxDevSoilMoisture, &flxDevSoilMoisture::get_cal_low, &flxDevSoilMoisture::set_cal_low>
        calibrationDry = {0};
    flxPropertyRWUInt16<flxDevSoilMoisture, &flxDevSoilMoisture::get_cal_high, &flxDevSoilMoisture::set_cal_high>
        calibrationWet = {1024};

    // functions
    flxParameterInVoid<flxDevSoilMoisture, &flxDevSoilMoisture::calibrate_low_value> calibrateLowValue;
    flxParameterInVoid<flxDevSoilMoisture, &flxDevSoilMoisture::calibrate_high_value> calibrateHighValue;

    // Define our output parameters - specify the get functions to call.
    flxParameterOutUInt16<flxDevSoilMoisture, &flxDevSoilMoisture::read_moisture_value> moistureValue;
    flxParameterOutFloat<flxDevSoilMoisture, &flxDevSoilMoisture::read_moisture_percent> moisturePercent;
};
