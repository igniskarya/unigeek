//
// M5Stack CoreS3 — AW88298 class-D I2S speaker amp.
// Amp power gate via AW9523B P0.2 (I2C 0x58).
// AW88298 configured via I2C 0x36 on Wire1 (internal bus, already begun by Device).
// I2S pins: BCK=34, WS=33, DOUT=13 on I2S_NUM_1.
//
// Volume: quadratic software curve, hardware fixed at 0x0032 (half of M5Unified full).
//   5% floor so vol=1 is always audible. vol=0 → silence.
//
// Init order: SpeakerI2S::begin() FIRST (starts BCK), then _initAW88298().
//   AW88298 must see stable BCK when its registers are written — configuring it before
//   BCK is running leaves the I2S receiver unlocked and produces no tone output.
//   M5Unified's enable callback fires the same way: after I2S is already running.
//
// No tone() override — any I2C write to AW88298 while BCK is active causes a brief
//   I2S sync loss that swallows the 150ms beep. AW88298 stays configured from begin().
//

#pragma once

#include "core/SpeakerI2S.h"
#include "../lib/AW9523B.h"
#include <Wire.h>

class SpeakerCoreS3 : public SpeakerI2S
{
public:
  explicit SpeakerCoreS3(AW9523B* aw) : _aw(aw) {}

  void begin() override {
    _aw->setSpeakerEnable(true);
    SpeakerI2S::begin();  // I2S driver up, BCK running, setVolume() dispatched here
    _initAW88298();       // configure AW88298 AFTER BCK is stable
  }

  // Quadratic curve: vol=30% → 9% amplitude (M5Unified's default is ~1.6%).
  // This maps the 0-100% slider to a perceptually sensible loudness range.
  void setVolume(uint8_t vol) override {
    if (vol == 0) { SpeakerI2S::setVolume(0); return; }
    float f = vol / 100.0f;
    int32_t amp = (int32_t)(f * f * 32767.0f);
    if (amp < 1638) amp = 1638;   // 5% floor — vol=1 stays audible
    if (amp > 32767) amp = 32767;
    SpeakerI2S::setVolume((uint8_t)(amp * 100 / 32767));
  }

private:
  AW9523B* _aw;
  static constexpr uint8_t AW88298_ADDR = 0x36;

  void _initAW88298() {
    _aw88298Write(0x61, 0x0673);  // boost mode disabled
    _aw88298Write(0x04, 0x4040);  // I2SEN=1, amp enable
    _aw88298Write(0x05, 0x0008);  // HMUTE=0, normal operation
    _aw88298Write(0x06, 0x14C7);  // clock: 44100 Hz, BCK=16×2
    _aw88298Write(0x0C, 0x0032);  // hardware volume = half of M5Unified full (0x64→0x32)
  }

  // Wire1 is the internal I2C bus — already begun by Device::createInstance()
  void _aw88298Write(uint8_t reg, uint16_t val) {
    Wire1.beginTransmission(AW88298_ADDR);
    Wire1.write(reg);
    Wire1.write((uint8_t)(val >> 8));
    Wire1.write((uint8_t)(val & 0xFF));
    Wire1.endTransmission();
  }
};