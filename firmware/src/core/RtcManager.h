//
// RtcManager.h — read/write hardware RTC via I2C (PCF8563 / PCF85063A)
//
// Requires pins_arduino.h to define:
//   DEVICE_HAS_RTC   — gates the entire implementation
//   RTC_I2C_ADDR     — 0x51 for both PCF8563 and PCF85063A
//   RTC_REG_BASE     — 0x02 for PCF8563, 0x04 for PCF85063A
//
// Both chips use identical 7-register BCD layout starting at RTC_REG_BASE:
//   +0 seconds  (mask 0x7F)
//   +1 minutes  (mask 0x7F)
//   +2 hours    (mask 0x3F)
//   +3 day      (mask 0x3F)
//   +4 weekday  (ignored on read, written as tm_wday)
//   +5 month    (mask 0x1F)
//   +6 year     (0–99 offset from 2000)
//

#pragma once

#ifdef DEVICE_HAS_RTC

#include <Wire.h>
#include <time.h>
#include <sys/time.h>

namespace RtcManager {

static inline uint8_t _bcd2bin(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
static inline uint8_t _bin2bcd(uint8_t v) { uint8_t h = v / 10; return (h << 4) | (v - h * 10); }

// Read RTC registers and set ESP32 system clock.
// Call once after Wire is ready (i.e. after Uni.begin()).
static inline void syncSystemFromRtc() {
  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write(RTC_REG_BASE);
  if (Wire.endTransmission() != 0) return;
  Wire.requestFrom((uint8_t)RTC_I2C_ADDR, (uint8_t)7);
  if (Wire.available() < 7) return;

  uint8_t sec  = _bcd2bin(Wire.read() & 0x7F);
  uint8_t min  = _bcd2bin(Wire.read() & 0x7F);
  uint8_t hour = _bcd2bin(Wire.read() & 0x3F);
  uint8_t day  = _bcd2bin(Wire.read() & 0x3F);
  Wire.read();  // weekday — skip
  uint8_t mon  = _bcd2bin(Wire.read() & 0x1F);
  uint8_t year = _bcd2bin(Wire.read());

  struct tm t = {};
  t.tm_sec   = sec;
  t.tm_min   = min;
  t.tm_hour  = hour;
  t.tm_mday  = day;
  t.tm_mon   = mon - 1;
  t.tm_year  = 2000 + year - 1900;
  t.tm_isdst = -1;

  time_t epoch = mktime(&t);
  if (epoch < 1577836800L) return;  // reject if before 2020-01-01 (RTC not yet set)

  struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
  settimeofday(&tv, nullptr);
}

// Write ESP32 system time (UTC) to RTC registers.
// Call after a successful NTP sync.
static inline void syncRtcFromSystem() {
  time_t now;
  time(&now);
  if (now < 1577836800L) return;  // don't write invalid time

  struct tm* t = gmtime(&now);

  Wire.beginTransmission(RTC_I2C_ADDR);
  Wire.write(RTC_REG_BASE);
  Wire.write(_bin2bcd(t->tm_sec));
  Wire.write(_bin2bcd(t->tm_min));
  Wire.write(_bin2bcd(t->tm_hour));
  Wire.write(_bin2bcd(t->tm_mday));
  Wire.write((uint8_t)t->tm_wday);
  Wire.write(_bin2bcd(t->tm_mon + 1));
  Wire.write(_bin2bcd((uint8_t)(t->tm_year - 100)));  // tm_year is years since 1900; store 0–99 from 2000
  Wire.endTransmission();
}

}  // namespace RtcManager

#endif  // DEVICE_HAS_RTC