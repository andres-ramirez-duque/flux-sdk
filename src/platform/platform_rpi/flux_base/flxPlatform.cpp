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

#include <pico/unique_id.h>

// rpi version of our platform class

const char *flxPlatform::unique_id(void)
{
    static char szDeviceID[13] = {0};
    if (szDeviceID[0] == 0)
    {
        pico_get_unique_board_id_string(szDeviceID, sizeof(szDeviceID));
    }
    return szDeviceID;
}