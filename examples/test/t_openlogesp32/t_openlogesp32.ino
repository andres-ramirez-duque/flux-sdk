/*
 *
 * Copyright (c) 2022-2024, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 */

/*
 * Spark Framework demo - logging
 *
 */

// Spark framework
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
#include <Flux/flxStorageJSONPref.h>

// Testing for device calls
#include <Flux/flxDevButton.h>

// SD Card output
#include <Flux/flxFSSDMMCard.h>
#include <Flux/flxFileRotate.h>

// WiFi Testing
#include <Flux/flxWiFiESP32.h>

// NTP
#include <Flux/flxNTPESP32.h>

// NFC device
#include <Flux/flxDevST25DV.h>

// SPI Devices
// The onboard IMU
#include <Flux/flxDevISM330.h>
static const uint8_t IMU_CS = 5;
// The onboard Magnetometer
#include <Flux/flxDevMMC5983.h>
static const uint8_t MAG_CS = 27;

// Biometric Hub
#include <Flux/flxDevBioHub.h>
static const uint8_t BIO_HUB_RESET = 17; // Use the TXD pin as the bio hub reset pin
static const uint8_t BIO_HUB_MFIO = 16;  // Use the RXD pin as the bio hub mfio pin

// MQTT
// #define TEST_MQTT
#include <Flux/flxMQTTESP32.h>

// AWS IoT
// UNCOMMENT TO TEST AWS
// #define TEST_AWS
#include <Flux/flxIoTAWS.h>

// ThingSpeak outpout
// Uncomment to test Thingspeak mqtt
// #define TEST_THINGSPEAK
#include <Flux/flxIoTThingSpeak.h>

// Azure IoT
// #define TEST_AZURE_IOT
#include <Flux/flxIoTAzure.h>

// HTTP output (call a URL with a JSON Payload)
// #define TEST_HTTP_IOT
#include <Flux/flxIoTHTTP.h>

#define OPENLOG_ESP32
#ifdef OPENLOG_ESP32
#define EN_3V3_SW 32
#define LED_BUILTIN 25
#endif

//------------------------------------------
// Default log interval in milli secs
#define kDefaultLogInterval 10000

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
flxFormatJSON<1200> fmtJSON;
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
flxStorageJSONPref jsonStorage;

flxWiFiESP32 wifiConnection;
flxNTPESP32 ntpClient;

// the onboard IMU
flxDevISM330_SPI onboardIMU;
flxDevMMC5983_SPI onboardMag;

// a biometric sensor hub
flxDevBioHub bioHub;

// An MQTT client
#ifdef TEST_MQTT
flxMQTTESP32 mqttClient;
#endif

#ifdef TEST_AWS
flxIoTAWS mqttAWS;
#endif

#ifdef TEST_THINGSPEAK
flxIoTThingSpeak iotThingSpeak;
#endif

#ifdef TEST_AZURE_IOT
flxIoTAzure iotAzure;
#endif

#ifdef TEST_HTTP_IOT
flxIoTHTTP iotHTTP;
#endif

//---------------------------------------------------------------------
void setupSDCard(void)
{
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

        // Serial.printf("SD card connected. Card Type: %s, Size: %uMB\n\r", theSDCard.type(), theSDCard.size());
    }
    // else
    //  Serial.println("SD card output not available");
}
//---------------------------------------------------------------------
// Check if we have a NFC reader available -- for use with WiFi credentials
//
// Call after autoload
void setupNFC(void)
{

    // do we have a NFC device connected?
    auto nfcDevices = flux.get<flxDevST25DV>();

    if (nfcDevices->size() == 0)
        return;

    Serial.println("We have an NFC reader attached.");

    // We have an NFC device. Create a credentials action and connect to the NFC device
    // and WiFi.
    flxSetWifiCredentials *pCreds = new flxSetWifiCredentials;

    if (!pCreds)
        return;

    pCreds->setCredentialSource(nfcDevices->at(0));
    pCreds->setWiFiDevice(&wifiConnection);

    // Change the name on the action

    pCreds->setName("WiFi Login From NFC", "Set the devices WiFi Credentials from an attached NFC source.");
}
//---------------------------------------------------------------------
void setupSPIDevices()
{
    // Note - framework is setting up the pins ...
    // IMU
    if (onboardIMU.initialize(IMU_CS))
    {
        flxLog_I(F("Onboard IMU is enabled"));
        logger.add(onboardIMU);
    }
    else
        flxLog_E(F("Onboard IMU failed to start."));

    // Magnetometer
    if (onboardMag.initialize(MAG_CS))
    {
        flxLog_I(F("Onboard Magnetometer is enabled"));
        logger.add(onboardMag);
    }
    else
        flxLog_E(F("Onboard Magnetometer failed to start"));
}
//---------------------------------------------------------------------
void setupBioHub()
{
    if (bioHub.initialize(BIO_HUB_RESET, BIO_HUB_MFIO)) // Initialize the bio hub using the reset and mfio pins,
    {
        flxLog_I(F("Bio Hub is enabled"));
        logger.add(bioHub);
    }
    else
        flxLog_E(F("Bio Hub not started/connected"));
}

//---------------------------------------------------------------------
void setupMQTT()
{

    // // Setup our MQTT parameters for testing if desired - or perform via settings/json file on SD card
    // mqttClient.server = "duke.home.lan";
    // mqttClient.topic = "/openlogesp32/logger1";
    // mqttClient.clientName = "myesp32";

#ifdef TEST_MQTT
    // setup the network connection for the mqtt
    mqttClient.setNetwork(&wifiConnection);

    // add mqtt to JSON
    fmtJSON.add(mqttClient);

#endif
#ifdef TEST_AWS
    // AWS
    mqttAWS.setName("AWS IoT", "Connect to an AWS Iot Thing");
    mqttAWS.setNetwork(&wifiConnection);

    // Add the filesystem to load certs/keys from the SD card
    mqttAWS.setFileSystem(&theSDCard);

    // Note: Certs and settings are loaded off the SD card at startup.

    fmtJSON.add(mqttAWS);
#endif

#ifdef TEST_THINGSPEAK

    iotThingSpeak.setNetwork(&wifiConnection);

    // Add the filesystem to load certs/keys from the SD card
    iotThingSpeak.setFileSystem(&theSDCard);

    // Note: Certs and settings are loaded off the SD card at startup.

    fmtJSON.add(iotThingSpeak);

#endif

#ifdef TEST_AZURE_IOT

    iotAzure.setNetwork(&wifiConnection);

    // Add the filesystem to load certs/keys from the SD card
    iotAzure.setFileSystem(&theSDCard);

    // Note: Certs and settings are loaded off the SD card at startup.

    fmtJSON.add(iotAzure);

#endif

#ifdef TEST_HTTP_IOT

    iotHTTP.setNetwork(&wifiConnection);
    iotHTTP.setFileSystem(&theSDCard);
    fmtJSON.add(iotHTTP);
#endif
}
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
        // Serial.println("\n---- Startup ----");

#ifdef OPENLOG_ESP32
    pinMode(EN_3V3_SW, OUTPUT); // Enable Qwiic power and I2C
    digitalWrite(EN_3V3_SW, HIGH);
#endif

    // Lets set the application name?!
    flux.setName("SparkFun Data Logger ESP32", "(c) 2023 SparkFun Electronics");
    flux.setVersion("0.9.1 Alpha", 10009001);

    // If not using settings, can use the following lines to test WiFi manually
    // Try WiFi
    // wifiConnection.SSID = "";
    // wifiConnection.password = "";

    // set the settings storage system for spark
    flxSettings.setStorage(&myStorage);
    flxSettings.setFallback(&jsonStorage);
    jsonStorage.setFileSystem(&theSDCard);
    jsonStorage.setFilename("openlog.json");

    // Have settings saved when editing is complete.
    flxSettings.listenForSave(serialSettings.on_finished);

    // Add serial settings to spark - the spark loop call will take care
    // of everything else.
    flux.add(serialSettings);

    // wire up the NTP to the wifi network object. When the connection status changes,
    // the NTP client will start and stop.
    ntpClient.setNetwork(&wifiConnection);
    ntpClient.setStartupDelay(5); // Give the NTP server some time to start

    // setup SD card. Do this before calling start - so prefs can be read off SD if needed
    setupSDCard();

    // setup MQTT
    setupMQTT();

    // Start Spark - Init system: auto detects devices and restores settings from EEPROM
    //               This should be done after all devices are added..for now...
    flux.start();

    flxLog_I(F("Device ID: %s"), flux.deviceId());

    // Write out the SD card stats
    if (theSDCard.enabled())
        flxLog_I(F("SD card available. Type: %s, Size: %uMB"), theSDCard.type(), theSDCard.size());
    else
        flxLog_W(F("SD card not available."));

    // WiFi status
    if (!wifiConnection.isConnected())
        flxLog_E(F("Unable to connect to WiFi!"));

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

    setupNFC();

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

    flxLog_I(F("Devices Detected [%d]"), myDevices.size());

    // Loop over the device list - note that it is iterable.
    for (auto device : myDevices)
    {
        flxLog_N_(F("\tDevice: %s, Output Number: %d"), device->name(), device->nOutputParameters());
        if (device->nOutputParameters() > 0)
        {
            flxLog_N(F("  - Adding to logger"));
            logger.add(device);
        }
        else
            flxLog_N(F(" - Not adding to logger"));
    }

    // Setup the Onboard IMU
    setupSPIDevices();

    // Setup the Bio Hub
    setupBioHub();

    ////////////
    // getAll() testing
    // auto allButtons = flux.get<flxDevButton>();

    // Serial.printf("Number of buttons: %d \n\r", allButtons->size());
    // for( auto button: *allButtons)
    // {
    //     Serial.printf("Button Name: %s", button->name());

    //     // Have the button trigger a log entry
    //     logger.listen(button->on_clicked);

    //     // Lets long the value of the button event
    //     logger.listenLogEvent(button->on_clicked, button);
    // }

    /// END TESTING
    digitalWrite(LED_BUILTIN, LOW); // board LED off

    flxLog_N("\n\rLog Output:");
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
    delay(500);
    digitalWrite(LED_BUILTIN, LOW); // turn off the log led
    delay(1000);
}
