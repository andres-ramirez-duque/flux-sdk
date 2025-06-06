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
 *  flxDevTwist.h
 *
 *  Device object for the Qwiic Twist device.
 *
 *
 */

#pragma once

#include "Arduino.h"

#include "SparkFun_Qwiic_Twist_Arduino_Library.h"
#include "flxCoreJobs.h"
#include "flxDevice.h"

// What is the name used to ID this device?
#define kTwistDeviceName "TWIST"
//----------------------------------------------------------------------------------------------------------
// Define our class - note we are sub-classing from the Qwiic Library
class flxDevTwist : public flxDeviceI2CType<flxDevTwist>, public TWIST
{

  public:
    flxDevTwist();

    //-----------------------------------------------------------------
    // Static Interface - used by the system to determine if this device is
    // connected before the object is instantiated.
    static bool isConnected(flxBusI2C &i2cDriver, uint8_t address);

    static flxDeviceConfidence_t connectedConfidence(void)
    {
        return flxDevConfidenceFuzzy;
    }

    static const char *getDeviceName()
    {
        return kTwistDeviceName;
    };
    static const uint8_t *getDefaultAddresses()
    {
        return defaultDeviceAddress;
    }
    // holds the class list of possible addresses/IDs for this objects
    static uint8_t defaultDeviceAddress[];

    //-----------------------------------------------------------------
    // Method called to initialize the class
    bool onInitialize(TwoWire &);

    // Events
    // Our output event
    flxSignalBool on_clicked;
    flxSignalInt32 on_twist;

  private:
    void checkTwist(void);
    int _last_count = 0;

    bool _pressMode = true;
    bool _last_button_state = false;
    bool _this_button_state = false;
    bool _toggle_state = false;

    // methods used to get values for our output parameters
    bool read_button_state();
    int32_t get_twist_count();

    // methods for our read-write properties
    uint8_t get_press_mode();
    void set_press_mode(uint8_t);
    uint8_t get_led_red();
    void set_led_red(uint8_t);
    uint8_t get_led_green();
    void set_led_green(uint8_t);
    uint8_t get_led_blue();
    void set_led_blue(uint8_t);

    uint8_t _ledRed = 128;
    uint8_t _ledGreen = 0;
    uint8_t _ledBlue = 128;

    flxJob _theJob;

  public:
    flxPropertyRWUInt8<flxDevTwist, &flxDevTwist::get_press_mode, &flxDevTwist::set_press_mode> pressMode = {
        1, {{"Click (Toggle) Mode", 0}, {"Press Mode", 1}}};
    flxPropertyRWUInt8<flxDevTwist, &flxDevTwist::get_led_red, &flxDevTwist::set_led_red> ledRed;
    flxPropertyRWUInt8<flxDevTwist, &flxDevTwist::get_led_green, &flxDevTwist::set_led_green> ledGreen;
    flxPropertyRWUInt8<flxDevTwist, &flxDevTwist::get_led_blue, &flxDevTwist::set_led_blue> ledBlue;

    // Define our output parameters - specify the get functions to call.
    flxParameterOutBool<flxDevTwist, &flxDevTwist::read_button_state> buttonState;
    flxParameterOutInt32<flxDevTwist, &flxDevTwist::get_twist_count> twistCount;
};
