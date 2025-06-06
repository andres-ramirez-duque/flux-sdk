/*
 *
 * Copyright (c) 2022-2024, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 */

/*
 * Flux Framework demo - simple logging output on a ESP32 device
 *
 * Note: This demo requires the generation of a SparkFun_Flux Arduino library - that is then installed
 *      into the Arduino libraries folder, or via the --library option in the Arduino CLI.
 *
 */

// Flux framework
#include <Flux.h>
#include <Flux/flxFmtCSV.h>
#include <Flux/flxFmtJSON.h>
#include <Flux/flxLogger.h>
#include <Flux/flxSerial.h>
#include <Flux/flxTimer.h>

// settings storage
#include <Flux/flxSettings.h>
#include <Flux/flxSettingsSerial.h>
#include <Flux/flxStorageESP32Pref.h>

// Testing for device calls
#include <Flux/flxDevButton.h>

// SD Card output
#include <Flux/flxFSSDMMCard.h>
#include <Flux/flxFileRotate.h>

// WiFi Testing
#include <Flux/flxWiFiESP32.h>

// NTP
#include <Flux/flxNTPESP32.h>

//------------------------------------------
// Default log interval in milli secs
#define kDefaultLogInterval 6000

/////////////////////////////////////////////////////////////////////////
// Spark Framework
/////////////////////////////////////////////////////////////////////////
// Spark Structure and Object Definition
//
// This app implements a "logger", which grabs data from
// connected devices and writes it to the Serial Console

// Create a logger action and add:
//   - Output devices: Serial

// Note - these could be added later using the add() method on logger

// Create a JSON and CSV output formatters.
// Note: setting internal buffer sizes using template to minimize alloc calls.
flxFormatJSON<1000> fmtJSON;
flxFormatCSV fmtCSV;

flxLogger logger;

// Enable a timer with a default timer value - this is the log interval
flxTimer timer(kDefaultLogInterval); // Timer

// SD Card Filesystem object
flxFSSDMMCard theSDCard;

// A writer interface for the SD Card that also rotates files
flxFileRotate theOutputFile;

// settings things
flxStorageESP32Pref myStorage;
flxSettingsSerial serialSettings;

flxWiFiESP32 wifiConnection;
flxNTPESP32 ntpClient;

//---------------------------------------------------------------------
// Arduino Setup
//
void setup()
{

    // Begin setup - turn on board LED during setup.
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.begin(115200);
    while (!Serial)
        ;
    Serial.println("\n---- Startup ----");

    // If not using settings, can use the following lines to test WiFi manually
    // Try WiFi
    // wifiConnection.SSID = "";
    // wifiConnection.password = "";

    // set the settings storage system for spark
    flxSettings.setStorage(&myStorage);

    // Have settings saved when editing is complete.
    flxSettings.listenForSave(serialSettings.on_finished);

    // Add serial settings to spark - the spark loop call will take care
    // of everything else.
    flux.add(serialSettings);

    // wire up the NTP to the wifi network object. When the connection status changes,
    // the NTP client will start and stop.
    ntpClient.setNetwork(&wifiConnection);
    ntpClient.setStartupDelay(5); // Give the NTP server some time to start

    // Start Spark - Init system: auto detects devices and restores settings from EEPROM
    //               This should be done after all devices are added..for now...
    flux.start();

    if (wifiConnection.isConnected())
        Serial.println("Connected to Wifi!");
    else
        Serial.println("Unable to connect to WiFi!");

    // Logging is done at an interval - using an interval timer.
    // Connect logger to the timer event
    logger.listen(timer.on_interval);

    // We want to output JSON and CSV to the serial consol.
    //  - Add Serial to our  formatters
    fmtJSON.add(flxSerial());
    fmtCSV.add(flxSerial());

    //  - Add the JSON and CVS format to the logger
    logger.add(fmtJSON);
    logger.add(fmtCSV);

    // setup output to the SD card
    if (theSDCard.initialize())
    {

        theOutputFile.setName("Data File", "Output file rotation manager");

        // SD card is available - lets setup output for it
        // Add the filesystem to the file output/rotation object
        theOutputFile.setFileSystem(theSDCard);

        // setup our file rotation parameters
        theOutputFile.filePrefix = "sfe";
        theOutputFile.startNumber = 1;
        theOutputFile.rotatePeriod(24); // one day

        // add the file output to the CSV output.
        fmtCSV.add(theOutputFile);
        // have the CSV formatter listen to the new file event. This
        // will cause a header to be written next cycle.
        fmtCSV.listenNewFile(theOutputFile.on_newFile);

        Serial.printf("SD card connected. Card Type: %s, Size: %uMB\n\r", theSDCard.type(), theSDCard.size());
    }
    else
        Serial.println("SD card output not available");

    // What devices has the system detected?
    // List them and add them to the logger

    flxDeviceContainer myDevices = flux.connectedDevices();

    // The device list can be added directly to the logger object using an
    // add() method call. This will only add devices with output parameters.
    //
    // Example:
    //      logger.add(myDevices);
    //
    // But for this example, let's loop over our devices and show how use the
    // device parameters.

    Serial.printf("Number of Devices Detected: %d\r\n", myDevices.size());

    // Loop over the device list - note that it is iterable.
    for (auto device : myDevices)
    {
        Serial.printf("\tDevice: %s, Output Number: %d", device->name(), device->nOutputParameters());
        if (device->nOutputParameters() > 0)
        {
            Serial.printf("  - Adding to logger\r\n");
            logger.add(device);
        }
        else
            Serial.printf(" - Not adding to logger \r\n");
    }

    ////////////
    // getAll() testing
    auto allButtons = flux.get<flxDevButton>();

    Serial.printf("Number of buttons: %d \n\r", allButtons->size());
    for (auto button : *allButtons)
    {
        Serial.printf("Button Name: %s", button->name());

        // Have the button trigger a log entry
        logger.listen(button->on_clicked);

        // Lets long the value of the button event
        logger.listenLogEvent(button->on_clicked, button);
    }

    /// END TESTING
    digitalWrite(LED_BUILTIN, LOW); // board LED off

    Serial.printf("\n\rLog Output:\n\r");
}

//---------------------------------------------------------------------
// Arduino loop -
void loop()
{

    ///////////////////////////////////////////////////////////////////
    // Spark
    //
    // Just call the spark framework loop() method. Spark will manage
    // the dispatch of processing to the components that were added
    // to the system during setup.
    if (flux.loop()) // will return true if an action did something
        digitalWrite(LED_BUILTIN, HIGH);

    // Our loop delay
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW); // turn off the log led
    delay(1000);
}
