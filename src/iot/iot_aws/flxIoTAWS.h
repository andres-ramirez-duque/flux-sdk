/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2024, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

#pragma once

#include "flxMQTTESP32.h"

#define kAWSUpdateShadowTemplate "{\"state\":{\"reported\":%s}}"
#define kAWSUpdateShadowTopic "$aws/things/%s/shadow/update"

// simple class to support AWS IoT

class flxIoTAWS : public flxMQTTESP32SecureCore<flxIoTAWS>, public flxWriter
{
  public:
    flxIoTAWS()
    {
        setName("AWS IoT", "Connection to AWS IoT");
        flux.add(this);
    }

    // for the Writer interface
    void write(int32_t data)
    {
        // noop
    }
    void write(float data)
    {
        // noop
    }
    virtual void write(const char *value, bool newline, flxLineType_t type)
    {
        // no data? Or is this a header line (not sure why it would be - we just want JSON)
        if (!value || type != flxLineTypeData)
            return;

        // Wrap the value with the structure required to update the device shadow
        char szBuffer[strlen(value) + sizeof(kAWSUpdateShadowTemplate)];
        snprintf(szBuffer, sizeof(szBuffer), kAWSUpdateShadowTemplate, value);
        flxMQTTESP32SecureCore::write(szBuffer, false, type);
    }

    bool initialize(void)
    {

        flxMQTTESP32SecureCore::initialize();

        char szBuffer[clientName().length() + sizeof(kAWSUpdateShadowTopic)];
        snprintf(szBuffer, sizeof(szBuffer), kAWSUpdateShadowTopic, clientName().c_str());

        topic = szBuffer;

        return true;
    }
};