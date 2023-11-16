/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2023, SparkFun Electronics Inc.  All rights reserved.
 * This software includes information which is proprietary to and a
 * trade secret of SparkFun Electronics Inc.  It is not to be disclosed
 * to anyone outside of this organization. Reproduction by any means
 * whatsoever is  prohibited without express written permission.
 *
 *---------------------------------------------------------------------------------
 */
/*
 *
 * flxDevice Objects
 *
 */

#include "flxCoreDevice.h"
#include "flxFlux.h"

///////////////////////////////////////////////////////////////////////////////////////
// Device factory things

// Handy macro helpers for the device multi-map key creation
#define devAddrToKey(__addr__, __conf__) ((__addr__ * 10 + (uint16_t)__conf__))
#define devKeyToAddr(__key__) (__key__ / 10)

///////////////////////////////////////////////////////////////////////////////////////
// Base Device class Impl
///////////////////////////////////////////////////////////////////////////////////////

bool flxDevice::initialize()
{
    flux.add(this);

    return true;
}

// Input param/function methods to enable/disable all parameters
void flxDevice::disable_all_parameters(void)
{
    for (auto param : getOutputParameters())
        param->setEnabled(false);
}

void flxDevice::enable_all_parameters(void)
{
    for (auto param : getOutputParameters())
        param->setEnabled(true);
}

//----------------------------------------------------------------
// Add the address to the device name. Helps ID a device
void flxDevice::addAddressToName()
{
    // add the device address to the name of the device.
    // Does it already exist?

    if (strchr(name(), '[') != NULL)
        return;

    char szBuffer[64];
    snprintf(szBuffer, sizeof(szBuffer), getKind() == flxDeviceKindSPI ? "%s [p%d]" : "%s [x%x]", name(), address());

    setNameAlloc(szBuffer);
}

///////////////////////////////////////////////////////////////////////////////////////
// Device Factory
///////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------
bool flxDeviceFactory::addressInUse(uint8_t address)
{
    // loop over connected/created devices - if the address is a match, return true
    for (auto device : flux.connectedDevices())
    {
        if (device->address() == address)
            return true;
    }
    return false;
}
///////////////////////////////////////////////////////////////////////////////////////
// The callback Builders use to register themselves at startup

bool flxDeviceFactory::registerDevice(flxDeviceBuilderI2C *deviceBuilder)
{
    // Add the builder to the device map - the map key is the address + a confidence value, and sorted

    flxDeviceConfidence_t devConfidence = deviceBuilder->connectedConfidence();

    // loop over the available addresses for this device ...
    const uint8_t *devAddr = deviceBuilder->getDefaultAddresses();

    uint16_t devKey;

    for (int i = 0; devAddr[i] != kSparkDeviceAddressNull; i++)
    {
        // key for the multi-map
        devKey = devAddrToKey(devAddr[i], devConfidence);

        // if the confidence type is PING, we can only really have one ping device per address.
        // Make sure we don't have two - this would be ambiguous...

        if (devConfidence == flxDevConfidencePing)
        {
            auto search = _buildersByAddress.find(devKey);
            if (search != _buildersByAddress.end())
            {
                // we have two pings - wut?
                flxLog_E(F("Unable to support the device %s. The address is ambiguous with %s"),
                         deviceBuilder->getDeviceName(), search->second->getDeviceName());
                continue;
            }
        }
        // Add this address-key <> builder pair to our multimap. Values are sorted by key
        _buildersByAddress.insert(std::pair<uint16_t, flxDeviceBuilderI2C *>(devKey, deviceBuilder));
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////
// buildConnectedDevices()
//
// Walks through the list of registered drivers and determines if the device is
// connected to the system. If it is, a driver is created and added to our driver list.
//
// Once this is completed, the "registered builders" list is cleared. This frees up the list,
// but the builder objects, which are globals (and small) remain.
//
// Return Value
//    The count of devices connected and the driver was successfully created...
///////////////////////////////////////////////////////////////////////////////////////

int flxDeviceFactory::buildDevices(flxBusI2C &i2cDriver)
{

    // walk the list of registered drivers
    int nDevs = 0;
    uint8_t devAddr;

    auto it = _buildersByAddress.begin();
    while (it != _buildersByAddress.end())
    {
        // Only autoload i2c devices
        if (it->second->getDeviceKind() != flxDeviceKindI2C)
            continue;

        // Get the devices I2C address;
        devAddr = devKeyToAddr(it->first);

        // address in use? Jump ahead
        if (addressInUse(devAddr))
        {
            // skip head to the next address block - follows the (address + ping) key in the map
            it = _buildersByAddress.upper_bound(devAddrToKey(devAddr, flxDevConfidencePing));
            continue;
        }

        // Is this device at this address?
        if (it->second->isConnected(i2cDriver, devAddr))
        {
            // yes connected - build a device driver
            flxDevice *pDevice = it->second->create();

            if (!pDevice)
                flxLog_E(F("Device create failed - %s"), it->second->getDeviceName());
            else
            {
                // setup the device object.
                pDevice->setName(it->second->getDeviceName());
                pDevice->setAddress(devAddr);
                pDevice->setAutoload();

                // call device initialize...
                if (!pDevice->initialize(i2cDriver))
                {
                    // device failed to init - delete it ...
                    flxLog_E(F("Deviced %s failed to initialize."), it->second->getDeviceName());
                    it->second->destroy(pDevice);
                }
                else
                {
                    // the device is added - skip to next address block - just after (the address + PING) key
                    it = _buildersByAddress.upper_bound(devAddrToKey(devAddr, flxDevConfidencePing));
                    nDevs++;
                    continue;
                }
            }
        }

        // okay, device not connected, or failed to init - check the next device in the list
        it++;
    }

    // done - no longer need the builders list/data
    _buildersByAddress.clear();
    return nDevs;
}

//----------------------------------------------------------------------------------
// pruneAutoload()
//
// Called when a non-autoload device is created.
//
// If a new device is created by the user outside of this factory, but that
// device was "auto loaded", we prune the autoload device.
//
// A device match = Device::type is the same and the address is the same.

void flxDeviceFactory::pruneAutoload(flxDevice *theDevice, flxDeviceContainer &devList)
{

    if (theDevice->autoload() || devList.size() == 0)
        return; // makes no sense.

    auto itDevice = devList.begin(); // get the iterator for the list

    while (itDevice != devList.end())
    {
        // only check auto loads
        if ((*itDevice)->autoload())
        {
            if (theDevice->getType() == (*itDevice)->getType() && theDevice->address() == (*itDevice)->address())
            {
                // remove the device - returns updated iterator
                flxDevice *pTmp = *itDevice;
                itDevice = devList.erase(itDevice);
                delete pTmp;
                break;
            }
        }
        itDevice++;
    }
}