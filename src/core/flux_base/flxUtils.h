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

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <time.h>

// use a utils namespace

namespace flx_utils
{

size_t dtostr(double value, char *szBuffer, size_t nBuffer, uint8_t precision = 3);

uint32_t id_hash_string(const char *str);

bool id_hash_string_to_string(const char *instr, char *outstr, size_t len);

//-------------------------------------------------------------------
std::string &to_string(std::string &data);
const std::string &to_string(std::string const &data);
std::string to_string(char *const data);
std::string to_string(int32_t const data);
std::string to_string(int8_t const data);
std::string to_string(int16_t const data);
std::string to_string(uint32_t const data);
std::string to_string(uint8_t const data);
std::string to_string(uint16_t const data);
std::string to_string(float const data, uint8_t precision);
std::string to_string(float const data);
std::string to_string(double const data, uint8_t precision);
std::string to_string(double const data);
std::string to_string(bool const data);

std::string strtrim(const std::string &str, const std::string &whitespace = " \t");

void encode_data(uint8_t *source, uint8_t *dest, size_t len, uint32_t key);
void decode_data(uint8_t *source, uint8_t *dest, size_t len, uint32_t key);

// AES encode/decode
bool encode_data_aes(uint8_t *key, unsigned char iv[16], char *source, char *output, size_t len);
bool decode_data_aes(uint8_t *key, unsigned char iv[16], char *source, char *output, size_t len);

void uptime(uint32_t &days, uint32_t &hours, uint32_t &minutes, uint32_t &secs, uint32_t &mills);

void timestampISO8601(time_t &theTime, char *buffer, size_t length, bool bTZ = false);

void formatByteString(uint64_t nBytes, uint prec, char *szBuffer, size_t len);

bool createVariableName(const char *szInVariable, char *szOutVariable);

// CRC32 Calculation
uint32_t calc_crc32(uint32_t crc, const uint8_t *buf, uint32_t size);

bool base64_decode(const char *data_in, size_t len, char *output);

bool is_little_endian();

uint16_t swap_uint16(uint16_t val);
uint32_t swap_uint32(uint32_t val);

} // namespace flx_utils
