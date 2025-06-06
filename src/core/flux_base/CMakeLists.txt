#
# Copyright (c) 2022-2024, SparkFun Electronics Inc.
#
# SPDX-License-Identifier: MIT
#
# Add the source files for this directory
flux_sdk_add_source_files(
    flxBusI2C.cpp
    flxBusI2C.h
    flxBusSPI.cpp
    flxBusSPI.h
    flxCore.cpp
    flxCore.h
    flxCoreDevice.cpp
    flxCoreDevice.h
    flxDeviceValueTypes.h
    flxCoreEvent.h
    flxCoreEvent.cpp
    flxCoreEventID.h
    flxCoreInterface.h
    flxCoreJobs.cpp
    flxCoreJobs.h
    flxCoreLog.cpp
    flxCoreLog.h
    flxCoreMsg.cpp
    flxCoreMsg.h
    flxCoreParam.h
    flxCoreProps.h
    flxCoreTypes.h
    flxDevice.h
    flxFlux.h
    flxSerial.cpp
    flxSerial.h
    flxTimer.h
    flxUtils.cpp
    flxUtils.h
    spSpark.cpp
    flxPlatform.h)

# the flux include file is special - its our hook for the Arduino "library" being created
configure_file(Flux.h ${PROJECT_FLUX_DIRECTORY}/src COPYONLY)
