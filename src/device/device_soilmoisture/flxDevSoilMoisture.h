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

#include "SparkFun_Soil_Moisture_Sensor.h"
#include "flxDevice.h"

// What is the name used to ID this device?
#define kSoilMoistureDeviceName "Soil Moisture Sensor"
//----------------------------------------------------------------------------------------------------------
// Define our class - note we are sub-classing from the Qwiic Library
class flxDevSoilMoisture : public flxDeviceI2CType<flxDevSoilMoisture>, public SparkFunSoilMoistureSensor
{

  public:
    flxDevSoilMoisture();

    // Static Interface - used by the system to determine if this device is
    // connected before the object is instantiated.
    static bool isConnected(flxBusI2C &i2cDriver, uint8_t address);

    static flxDeviceConfidence_t connectedConfidence(void)
    {
        return flxDevConfidencePing;
    }

    static const char *getDeviceName()
    {
        return kSoilMoistureDeviceName;
    };

    static const uint8_t *getDefaultAddresses()
    {
        return defaultDeviceAddress;
    }
    // holds the class list of possible addresses/IDs for this objects
    static uint8_t defaultDeviceAddress[];

    // Method called to initialize the class
    bool onInitialize(TwoWire &);

  private:
    // methods used to get values for our output parameters
    uint16_t read_moisture_value();
    float read_moisture_percent();

  public:
    // Define our output parameters - specify the get functions to call.
    flxParameterOutUInt16<flxDevSoilMoisture, &flxDevSoilMoisture::read_moisture_value> moistureValue;
    flxParameterOutFloat<flxDevSoilMoisture, &flxDevSoilMoisture::read_moisture_percent> moisturePercent;
};
