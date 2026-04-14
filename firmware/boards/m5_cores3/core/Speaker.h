//
// M5Stack CoreS3 — AW88298 class-D I2S speaker amp.
// Amp power gate via AW9523B P0.2 (I2C 0x58).
// AW88298 configured via I2C 0x36 on Wire1 (internal bus, already begun by Device).
// I2S pins: BCK=34, WS=33, DOUT=13 on I2S_NUM_1.
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
    // Gate the AW88298 amp via AW9523B P0.2
    _aw->setSpeakerEnable(true);

    // Configure AW88298 over Wire1 (internal I2C, already initialised):
    //   reg 0x61 = boost mode disabled
    //   reg 0x04 = I2SEN=1 (I2S active), AMPPD=0 (not powered down), PWDN=0
    //   reg 0x05 = RMSE/HAGCE/HDCCE/HMUTE all 0 (normal operation)
    //   reg 0x06 = I2S clock for 44100 Hz (BCK=0 → 16×2, rate index=7 → 0x14C7)
    //   reg 0x0C = full-scale hardware volume (0x0064)
    _aw88298Write(0x61, 0x0673);
    _aw88298Write(0x04, 0x4040);
    _aw88298Write(0x05, 0x0008);
    _aw88298Write(0x06, 0x14C7);
    _aw88298Write(0x0C, 0x0064);

    SpeakerI2S::begin();
  }

private:
  AW9523B* _aw;
  static constexpr uint8_t AW88298_ADDR = 0x36;

  // Wire1 is the internal I2C bus — already begun by Device::createInstance()
  void _aw88298Write(uint8_t reg, uint16_t val) {
    Wire1.beginTransmission(AW88298_ADDR);
    Wire1.write(reg);
    Wire1.write((uint8_t)(val >> 8));
    Wire1.write((uint8_t)(val & 0xFF));
    Wire1.endTransmission();
  }
};
