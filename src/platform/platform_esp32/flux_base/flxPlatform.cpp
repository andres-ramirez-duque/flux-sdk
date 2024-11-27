/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2024, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

#include "flxPlatform.h"

// esp version of our platform class

const char *flxPlatform::unique_id(void)
{
    static char szDeviceID[13] = {0};
    if (szDeviceID[0] == 0)
    {
        uint64_t chipid = ESP.getEfuseMac();
        snprintf(szDeviceID, sizeof(szDeviceID), "%012llX", ESP.getEfuseMac());
    }
    return szDeviceID;
}