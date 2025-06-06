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

#ifdef ESP32

#include "flxCoreInterface.h"
#include "flxFS.h"
#include "flxFlux.h"
#include "flxNetwork.h"

#include <ArduinoMqttClient.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>

// A General MQTT client for the framework - for use on the ESP32
template <class Object, typename CLIENT> class flxMQTTESP32Base : public flxActionType<Object>
{
  private:
    // Enabled Property setter/getters
    void set_isEnabled(bool bEnabled)
    {
        // Any changes?
        if (_isEnabled == bEnabled)
            return;

        _isEnabled = bEnabled;

        // TODO check network connection availability ...
        if (_isEnabled)
            (void)connect();
        else
            disconnect();
    }

    //----------------------------------------------------------------
    bool get_isEnabled(void)
    {
        return _isEnabled;
    }
    //----------------------------------------------------------------------------

    void set_bufferSize(uint16_t buffSize)
    {
        if (buffSize > 0)
        {
            _mqttClient.setTxPayloadSize(buffSize);
            _dynamicBufferSize = buffSize;
        }
        _txBufferSize = buffSize;
    }
    //----------------------------------------------------------------------------
    uint16_t get_bufferSize(void)
    {
        return _txBufferSize;
    }

    // Event callback
    //----------------------------------------------------------------------------
    void onConnectionChange(bool bConnected)
    {
        // Are we enabled ...
        if (!_isEnabled)
            return;

        // Anything change?
        if (bConnected == connected())
            return;

        if (bConnected)
        {
            flxLog_I_(F("%s: connecting to MQTT endpoint %s:%u ..."), this->name(), server().c_str(), port());
            if (connect())
                flxLog_N(F("connected"));
            // the connect method will print out sufficient error messages
        }
        else
        {
            flxLog_I(F("Disconnecting from MQTT endpoint %s"), clientName().c_str());
            disconnect();
        }
    }

  public:
    flxMQTTESP32Base()
        : _isEnabled{false}, _theNetwork{nullptr}, _mqttClient(_wifiClient), _txBufferSize{0}, _dynamicBufferSize{0}
    {
        flxRegister(enabled, "Enabled", "Enable or Disable the MQTT Client");

        flxRegister(port, "Port", "The MQTT broker port to connect to");
        flxRegister(server, "Server", "The MQTT server to connect to");
        flxRegister(topic, "MQTT Topic", "The MQTT topic to publish to");
        flxRegister(clientName, "Client Name", "Name of this device used for MQTT Communications");

        flxRegister(username, "Username", "Username to connect to an MQTT broker, if required");
        flxRegister(password, "Password", "Password to connect to an MQTT broker, if required");

        flxRegister(bufferSize, "Buffer Size", "MQTT payload buffer size. If 0, the buffer size is dynamic");
    };

    ~flxMQTTESP32Base()
    {
        disconnect();
    }

    void setNetwork(flxNetwork *theNetwork)
    {
        _theNetwork = theNetwork;

        flxRegisterEventCB(flxEvent::kOnConnectionChange, this, &flxMQTTESP32Base::onConnectionChange);
    }

    bool connected()
    {
        return (_isEnabled && _wifiClient.connected() != 0 && _mqttClient.connected() != 0);
    }

    //----------------------------------------------------------------------------
    virtual void disconnect(void)
    {
        if (_mqttClient.connected() != 0)
            _mqttClient.stop();

        if (_wifiClient.connected() != 0)
            _wifiClient.stop();
    }
    //----------------------------------------------------------------------------
    virtual bool connect(void)
    {

        // if we don't have a network, or if the network is not connected, return an error
        if (!_theNetwork || !_theNetwork->isConnected())
            return false;

        // Already connected?
        if (connected())
            return true;

        // do we have all the parameters we need?
        if (clientName().length() == 0)
        {
            flxLogM_E(kMsgErrValueNotProvided, this->name(), "Thing Name");
            return false;
        }
        if (server().length() == 0)
        {
            flxLogM_E(kMsgErrValueNotProvided, this->name(), "Server/Endpoint");
            return false;
        }
        if (port() < 1024)
        {
            flxLogM_E(kMsgErrValueNotProvided, this->name(), "Valid port");
            return false;
        }
        // mqtt time

        _mqttClient.setId(clientName().c_str());
        _mqttClient.setKeepAliveInterval(60 * 1000);
        _mqttClient.setConnectionTimeout(20 * 1000);

        // Username/password provided?
        if (username().length() > 0 && password().length() > 0)
            _mqttClient.setUsernamePassword(username().c_str(), password().c_str());

        // Connect
        for (int i = 0; !_mqttClient.connect(server().c_str(), port()); i++)
        {
            if (i > 3)
            {
                flxLogM_E(kMsgErrConnectionFailureD, this->name(), _mqttClient.connectError());
                return false;
            }
            flxLog_N_(".");
            delay(400);
        }

        // we're connected
        return true;
    }

    //----------------------------------------------------------------------------
    // flxWriter interface method
    virtual void write(const char *value, bool newline, flxLineType_t type)
    {

        // Should we continue?
        // enabled? Have a value to send? We only deal with JSON - just the data
        if (!_isEnabled || !value || type != flxLineTypeData)
            return;

        // If we lost the connection to the broker, try to reconnect...
        if (!_mqttClient.connected() || !_wifiClient.connected())
        {
            flxLog_W_(F("%s disconnected - reconnecting..."), this->name());
            if (!connect())
                return;

            flxLog_N(F("reconnected"));
        }

        // do we have a topic?
        if (topic().length() == 0)
        {
            flxLog_E(kMsgErrValueNotProvided, this->name(), "MQTT Topic");
            return;
        }

        // the mqtt object has a limited transmit buffer size (256) that doesn't adapt,
        // but you can set the size (which performs a malloc and free)
        //
        // Openlog payloads can be large, so if in dynamic mode we keep track of the
        // allocated size and increase when needed if in dynamic buffer size mode ..

        if (_txBufferSize == 0 && _dynamicBufferSize < strlen(value))
        {
            _dynamicBufferSize = strlen(value);
            _mqttClient.setTxPayloadSize(_dynamicBufferSize);
        }

        // send the message
        _mqttClient.beginMessage(topic().c_str());
        _mqttClient.print(value);
        _mqttClient.endMessage();
    }
    // Properties

    // Enabled/Disabled
    flxPropertyRWBool<flxMQTTESP32Base, &flxMQTTESP32Base::get_isEnabled, &flxMQTTESP32Base::set_isEnabled> enabled;

    flxPropertyUInt32<flxMQTTESP32Base> port = {1883}; // default mqtt port
    flxPropertyString<flxMQTTESP32Base> server;
    flxPropertyString<flxMQTTESP32Base> topic;
    flxPropertyString<flxMQTTESP32Base> clientName;

    // Buffer size property
    flxPropertyRWUInt16<flxMQTTESP32Base, &flxMQTTESP32Base::get_bufferSize, &flxMQTTESP32Base::set_bufferSize>
        bufferSize = {0};

    // username and password properties - some brokers requires this
    flxPropertyString<flxMQTTESP32Base> username;
    flxPropertySecureString<flxMQTTESP32Base> password;

  protected:
    CLIENT _wifiClient;

  private:
    bool _isEnabled;

    flxNetwork *_theNetwork;

    MqttClient _mqttClient;

    uint16_t _txBufferSize;
    uint16_t _dynamicBufferSize;
};

class flxMQTTESP32 : public flxMQTTESP32Base<flxMQTTESP32, WiFiClient>, public flxWriter
{
  public:
    flxMQTTESP32()
    {
        this->setName("MQTT Client", "A generic MQTT Client");

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
    void write(const char *value, bool newline, flxLineType_t type)
    {
        flxMQTTESP32Base::write(value, newline, type);
    }
};

template <class Object> class flxMQTTESP32SecureCore : public flxMQTTESP32Base<Object, WiFiClientSecure>
{

  private:
    // NOTE:
    //   Standard string props are stored in std::string variables.
    //   Because the c_str() method on std::strings returns a value that is temporary,
    ///  and the underlying secure connection is expecting const chars, we use a RW
    //   property for the cert/key strings and stash the values in allocated memory.

    //---------------------------------------------------------
    std::string get_caCert(void)
    {

        std::string tmp = _pCACert ? _pCACert : "";
        return tmp;
    }

    //---------------------------------------------------------
    void set_caCert(std::string theCert)
    {
        if (_pCACert != nullptr)
        {
            delete _pCACert;
            _pCACert = nullptr;
        }

        if (theCert.length() > 0)
            _pCACert = strdup(theCert.c_str());
    }

    //---------------------------------------------------------
    std::string get_clientCert(void)
    {
        std::string tmp = _pClientCert ? _pClientCert : "";
        return tmp;
    }

    //---------------------------------------------------------
    void set_clientCert(std::string theCert)
    {
        if (_pClientCert != nullptr)
        {
            delete _pClientCert;
            _pClientCert = nullptr;
        }

        if (theCert.length() > 0)
            _pClientCert = strdup(theCert.c_str());
    }

    //---------------------------------------------------------
    std::string get_clientKey(void)
    {
        std::string tmp = _pClientKey ? _pClientKey : "";
        return tmp;
    }

    //---------------------------------------------------------
    void set_clientKey(std::string theCert)
    {
        if (_pClientKey != nullptr)
        {
            delete _pClientKey;
            _pClientKey = nullptr;
        }

        if (theCert.length() > 0)
            _pClientKey = strdup(theCert.c_str());
    }

    //---------------------------------------------------------
    std::string get_caCertFilename(void)
    {
        return _caFilename;
    }

    //---------------------------------------------------------
    void set_caCertFilename(std::string theFile)
    {
        char *pCert = loadCertFile(theFile);

        if (!pCert)
            return;

        if (_pCACert != nullptr)
            delete _pCACert;

        _pCACert = pCert;

        _caFilename = theFile;
    }

    //---------------------------------------------------------
    std::string get_clientCertFilename(void)
    {
        return _clientFilename;
    }

    //---------------------------------------------------------
    void set_clientCertFilename(std::string theFile)
    {
        char *pCert = loadCertFile(theFile);

        if (!pCert)
            return;

        if (_pClientCert != nullptr)
            delete _pClientCert;

        _pClientCert = pCert;

        _clientFilename = theFile;
    }

    //---------------------------------------------------------
    std::string get_clientKeyFilename(void)
    {
        return _keyFilename;
    }

    //---------------------------------------------------------
    void set_clientKeyFilename(std::string theFile)
    {
        char *pCert = loadCertFile(theFile);

        if (!pCert)
            return;

        if (_pClientKey != nullptr)
            delete _pClientKey;

        _pClientKey = pCert;

        _keyFilename = theFile;
    }

    //---------------------------------------------------------
    char *loadCertFile(std::string &theFile)
    {

        if (!_fileSystem || theFile.length() < 1)
            return nullptr;

        if (!_fileSystem->exists(theFile.c_str()))
        {
            flxLogM_E(kMsgErrFileOpen, this->name(), theFile.c_str());
            return nullptr;
        }

        flxFSFile certFile = _fileSystem->open(theFile.c_str(), flxIFileSystem::kFileRead);
        if (!certFile)
        {
            flxLogM_E(kMsgErrFileOpen, this->name(), theFile.c_str());
            return nullptr;
        }

        size_t szFile = certFile.size();
        if (szFile < 1)
        {
            certFile.close();
            flxLogM_E(kMsgErrFileOpen, this->name(), theFile.c_str());
            return nullptr;
        }
        uint8_t *pCert = new uint8_t[szFile + 1];

        if (!pCert)
        {
            certFile.close();
            flxLogM_E(kMsgErrAllocErrorN, this->name(), theFile.c_str());
            return nullptr;
        }

        size_t szRead = certFile.read(pCert, szFile);

        certFile.close(); // we are done with the file.

        if (szFile != szRead)
        {
            flxLogM_E(kMsgErrAllocErrorN, this->name(), theFile.c_str());
            delete pCert;
            return nullptr;
        }
        // null terminate the string
        pCert[szFile] = '\0';

        return (char *)pCert;
    }

  public:
    flxMQTTESP32SecureCore() : _pCACert{nullptr}, _pClientCert{nullptr}, _pClientKey{nullptr}, _fileSystem{nullptr}
    {
        flxRegister(caCertificate, "CA Certificate",
                    "Certificate Authority certificate. If set, the connection is secure");
        flxRegister(clientCertificate, "Client Certificate", "Certificate for the client connection");
        flxRegister(clientKey, "Client Key", "Secure key used for client verification");

        flxRegister(caCertFilename, "CA Cert Filename", "File to load the certificate from");
        flxRegister(clientCertFilename, "Client Cert Filename", "File to load the client certificate from");
        flxRegister(clientKeyFilename, "Client Key Filename", "File to load the client key from");

        // The default port for secure/TLS MQTT is 8883, so lest set that for this secure client
        flxMQTTESP32Base<Object, WiFiClientSecure>::port = 8883;
    }

    //---------------------------------------------------------
    ~flxMQTTESP32SecureCore()
    {
        if (_pCACert != nullptr)
            delete _pCACert;

        if (_pClientCert != nullptr)
            delete _pClientCert;

        if (_pClientKey != nullptr)
            delete _pClientKey;
    }

    //---------------------------------------------------------
    virtual bool connect(void)
    {
        // Already connected?
        if (flxMQTTESP32Base<Object, WiFiClientSecure>::connected())
            return true;

        if (_pCACert != nullptr)
            flxMQTTESP32Base<Object, WiFiClientSecure>::_wifiClient.setCACert(_pCACert);

        if (_pClientCert != nullptr)
            flxMQTTESP32Base<Object, WiFiClientSecure>::_wifiClient.setCertificate(_pClientCert);

        if (_pClientKey != nullptr)
            flxMQTTESP32Base<Object, WiFiClientSecure>::_wifiClient.setPrivateKey(_pClientKey);

        return flxMQTTESP32Base<Object, WiFiClientSecure>::connect();
    }

    //---------------------------------------------------------
    void setFileSystem(flxIFileSystem *fs)
    {
        _fileSystem = fs;
    }

    // Security certs/keys
    flxPropertyRWSecretString<flxMQTTESP32SecureCore, &flxMQTTESP32SecureCore::get_caCert,
                              &flxMQTTESP32SecureCore::set_caCert>
        caCertificate;

    flxPropertyRWSecretString<flxMQTTESP32SecureCore, &flxMQTTESP32SecureCore::get_clientCert,
                              &flxMQTTESP32SecureCore::set_clientCert>
        clientCertificate;

    flxPropertyRWSecretString<flxMQTTESP32SecureCore, &flxMQTTESP32SecureCore::get_clientKey,
                              &flxMQTTESP32SecureCore::set_clientKey>
        clientKey;

    // Define filename properties to access the secure keys. A filesystem must be provided to this object for it to read
    // the data.
    // Security certs/keys
    flxPropertyRWString<flxMQTTESP32SecureCore, &flxMQTTESP32SecureCore::get_caCertFilename,
                        &flxMQTTESP32SecureCore::set_caCertFilename>
        caCertFilename;
    flxPropertyRWString<flxMQTTESP32SecureCore, &flxMQTTESP32SecureCore::get_clientCertFilename,
                        &flxMQTTESP32SecureCore::set_clientCertFilename>
        clientCertFilename;
    flxPropertyRWString<flxMQTTESP32SecureCore, &flxMQTTESP32SecureCore::get_clientKeyFilename,
                        &flxMQTTESP32SecureCore::set_clientKeyFilename>
        clientKeyFilename;

    // We need perm version of the keys for the secure connection, so the values are stashed in allocated
    // strings
    char *_pCACert;
    char *_pClientCert;
    char *_pClientKey;

    // Filesystem to load a file from
    flxIFileSystem *_fileSystem;

    std::string _caFilename;
    std::string _clientFilename;
    std::string _keyFilename;
};

class flxMQTTESP32Secure : public flxMQTTESP32SecureCore<flxMQTTESP32Secure>, public flxWriter
{
  public:
    flxMQTTESP32Secure()
    {
        this->setName("MQTT Secure Client", "A secure MQTT client");

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
    void write(const char *value, bool newline, flxLineType_t type)
    {
        flxMQTTESP32Base::write(value, newline, type);
    }
};
#endif