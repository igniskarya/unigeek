//
// M5StickS3 — ES8311 codec (I2C 0x18) with M5PM1-gated class-D amp.
// M5PM1 GPIO3 gates the amp enable via official M5PM1 library.
// ES8311 must be configured via Wire1 before I2S audio is heard.
// Requires MCLK on GPIO 18. Extends SpeakerI2S.
//

#pragma once

#include "core/SpeakerI2S.h"
#include <driver/i2s.h>
#include <Wire.h>
#include <M5PM1.h>

extern M5PM1 pm1;

class SpeakerStickS3 : public SpeakerI2S
{
public:
  void begin() override
  {
    baseAmplitude = 5000;

    // 1. Configure M5PM1 GPIO3 as push-pull output HIGH (amp enable)
    pm1.gpioSet(M5PM1_GPIO_NUM_3, M5PM1_GPIO_MODE_OUTPUT, 1,
                M5PM1_GPIO_PULL_NONE, M5PM1_GPIO_DRIVE_PUSHPULL);

    // 2. Configure ES8311 codec (Wire1 already initialized by pm1.begin())
    _es8311Init();

    // 3. Install I2S driver with MCLK
    SpeakerI2S::begin();
    i2s_pin_config_t pins = {};
    pins.mck_io_num   = SPK_MCLK;
    pins.bck_io_num   = SPK_BCLK;
    pins.ws_io_num    = SPK_WCLK;
    pins.data_out_num = SPK_DOUT;
    pins.data_in_num  = I2S_PIN_NO_CHANGE;
    i2s_set_pin((i2s_port_t)SPK_I2S_PORT, &pins);
  }

private:
  // Configure ES8311 codec for DAC output (from M5Unified _speaker_enabled_cb_sticks3)
  static void _es8311Init() {
    static constexpr uint8_t ES8311_ADDR = 0x18;
    static constexpr uint8_t seq[][2] = {
      {0x00, 0x80},  // reset
      {0x01, 0xB5},  // clock: MCLK source, dividers
      {0x02, 0x18},  // clock: ADC/DAC rate
      {0x0D, 0x01},  // system: power-up
      {0x12, 0x00},  // DAC: no mute
      {0x13, 0x10},  // DAC: output enable
      {0x32, 0xBF},  // DAC volume: 0 dB
      {0x37, 0x08},  // DAC: ALC off, mono mix
    };
    for (auto& r : seq) {
      Wire1.beginTransmission(ES8311_ADDR);
      Wire1.write(r[0]);
      Wire1.write(r[1]);
      Wire1.endTransmission();
    }
  }
};
