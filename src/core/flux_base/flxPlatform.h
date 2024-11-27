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
// Defines a interface of static class methods that the *platform* must provide the implementation for

class flxPlatform
{
  public:
    // Return a unique identifier for the device - a 12 char hex string
    static const char *unique_id(void);
};