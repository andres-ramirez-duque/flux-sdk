/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2024, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

#include "flxFSSDCard.h"
#include <string>

#include "FS.h"
#include "SD.h"

// Helper to ensure full path for file names
static bool checkForFullPath(const char *filename, char *destbuffer, size_t length)
{
    if (!filename || length < 5)
        return false;

    uint16_t offset = 0;

    if (*filename != '/')
    {
        offset = 1;
        *destbuffer = '/';
    }
    snprintf(destbuffer + offset, length - offset, "%s", filename);

    return true;
}

_flxFSSDCard &_theSDCard = _flxFSSDCard::get();

bool _flxFSSDCard::initialize()
{
    if (_isInitalized)
        return true;

    if (_pinPower)
        pinMode(_pinPower, OUTPUT);

    pinMode(_pinCS, OUTPUT);
    digitalWrite(_pinCS, HIGH);

    setPower(true);
    delay(1000);

    if (!SD.begin(_pinCS))
    {
        flxLogM_E(kMsgErrDeviceInit, "SD Card", "start");
        return false;
    }

    _isInitalized = true;
    return true;
}

bool _flxFSSDCard::initialize(uint8_t pinCS)
{
    if (!pinCS)
        return false;

    _pinCS = pinCS;
    return initialize();
}

bool _flxFSSDCard::initialize(uint8_t pinCS, uint8_t pinPower)
{
    if (!pinCS || !pinPower)
        return false;

    _pinCS = pinCS;
    _pinPower = pinPower;
    return initialize();
}

void _flxFSSDCard::setPower(bool powerOn)
{
    if (!_pinPower || (_isInitalized && powerOn == _powerOn))
        return;

    pinMode(_pinPower, OUTPUT);
    digitalWrite(_pinPower, (powerOn ? LOW : HIGH));
    _powerOn = powerOn;
}

flxFSFile _flxFSSDCard::open(const char *name, flxFileOpenMode_t mode, bool create)
{
    flxFSFile theflxFile;
    if (!_isInitalized || !name || strlen(name) == 0)
        return theflxFile;

    const char *sdMode = "r";
    if (mode == kFileWrite)
        sdMode = "w+";
    else if (mode == kFileAppend)
        sdMode = "a+";

    char szBuffer[128];
    if (!checkForFullPath(name, szBuffer, sizeof(szBuffer)))
        return theflxFile;

    File sdFile = SD.open(szBuffer, sdMode);

    if (sdFile)
    {
        flxFSSDFile theFile;
        theFile.setFile(sdFile);
        std::shared_ptr<flxIFile> pFile = std::make_shared<flxFSSDFile>(std::move(theFile));
        theflxFile.setIFile(pFile);
    }
    else
        flxLogM_E(kMsgErrFileOpen, "SD Card", name);

    return theflxFile;
}

bool _flxFSSDCard::exists(const char *name)
{
    if (!_isInitalized)
        return false;

    char szBuffer[128];
    if (!checkForFullPath(name, szBuffer, sizeof(szBuffer)))
        return false;

    return SD.exists(szBuffer);
}

bool _flxFSSDCard::remove(const char *name)
{
    if (!_isInitalized)
        return false;

    char szBuffer[128];
    if (!checkForFullPath(name, szBuffer, sizeof(szBuffer)))
        return false;

    return SD.remove(szBuffer);
}

bool _flxFSSDCard::rename(const char *nameFrom, const char *nameTo)
{
    if (!_isInitalized)
        return false;

    char szBuffFrom[128];
    if (!checkForFullPath(nameFrom, szBuffFrom, sizeof(szBuffFrom)))
        return false;

    char szBuffTo[128];
    if (!checkForFullPath(nameTo, szBuffTo, sizeof(szBuffTo)))
        return false;

    return SD.rename(szBuffFrom, szBuffTo);
}

bool _flxFSSDCard::mkdir(const char *path)
{
    if (!_isInitalized)
        return false;

    char szBuffer[128];
    if (!checkForFullPath(path, szBuffer, sizeof(szBuffer)))
        return false;

    return SD.mkdir(szBuffer);
}

bool _flxFSSDCard::rmdir(const char *path)
{
    if (!_isInitalized)
        return false;

    char szBuffer[128];
    if (!checkForFullPath(path, szBuffer, sizeof(szBuffer)))
        return false;

    return SD.rmdir(szBuffer);
}

uint64_t _flxFSSDCard::size(void)
{
    if (!_isInitalized)
        return 0;

#if defined(ESP32)
    return SD.cardSize();
#else
    return SD.size64();
#endif
}

uint64_t _flxFSSDCard::total(void)
{
    if (!_isInitalized)
        return 0;

    return SD.totalBytes();
}

const char *_flxFSSDCard::type(void)
{
    if (!_isInitalized)
        return "Unknown";

#if defined(ESP32)
    switch (SD.cardType())
    {
    case CARD_MMC:
        return "MMC";
    case CARD_SD:
        return "SD";
    case CARD_SDHC:
        return "SDHC";
    case CARD_NONE:
        return "NO CARD";
    default:
        return "UNKNOWN";
    }
#else
    switch (SD.type())
    {
    case SD_CARD_TYPE_SD1:
        return "SD1";
    case SD_CARD_TYPE_SD2:
        return "SD2";
    case SD_CARD_TYPE_SDHC:
        return "SDHC";
    case 0:
        return "NO CARD";
    default:
        return "UNKNOWN";
    }
#endif
}

uint64_t _flxFSSDCard::used(void)
{
#if defined(ESP32)
    return SD.usedBytes();
#else
    FSInfo fs_info;
    SD.info(fs_info);
    return fs_info.usedBytes;
#endif
}

FS _flxFSSDCard::fileSystem(void)
{
    return SD;
}

// -----------------------------------------------------------------------------
// File implementation

bool flxFSSDFile::isValid()
{
    return _file;
}

size_t flxFSSDFile::write(const uint8_t *buf, size_t size)
{
    if (!_file)
        return 0;
    return _file.write(buf, size);
}

size_t flxFSSDFile::read(uint8_t *buf, size_t size)
{
    if (!_file)
        return 0;
    return _file.read(buf, size);
}

void flxFSSDFile::close(void)
{
    if (_file)
        _file.close();
}

void flxFSSDFile::flush(void)
{
    if (_file)
        _file.flush();
}

size_t flxFSSDFile::size(void)
{
    return _file ? _file.size() : 0;
}

const char *flxFSSDFile::name(void)
{
    return _file ? _file.name() : nullptr;
}

bool flxFSSDFile::isDirectory(void)
{
    return _file && _file.isDirectory();
}

std::string flxFSSDFile::getNextFilename(void)
{
    std::string tmp = "";
    if (_file)
    {
        File fNext = _file.openNextFile();
        if (fNext)
        {
            tmp = fNext.name();
            fNext.close();
        }
    }
    return tmp;
}

time_t flxFSSDFile::getLastWrite(void)
{
    return _file ? _file.getLastWrite() : 0;
}

flxFSFile flxFSSDFile::openNextFile(void)
{
    flxFSFile theflxFile;
    if (!_file || !_file.isDirectory())
        return theflxFile;

    File sdFile = _file.openNextFile();
    if (sdFile)
    {
        flxFSSDFile theFile;
        theFile.setFile(sdFile);
        std::shared_ptr<flxIFile> pFile = std::make_shared<flxFSSDFile>(std::move(theFile));
        theflxFile.setIFile(pFile);
    }
    return theflxFile;
}

int flxFSSDFile::available(void)
{
    return _file ? _file.available() : 0;
}

Stream *flxFSSDFile::stream(void)
{
    return _file ? &_file : nullptr;
}
