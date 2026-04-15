//
// M5Stack CoreS3 — AW88298 class-D I2S speaker amp.
// Amp power gate via AW9523B P0.2 (I2C 0x58).
// AW88298 configured via I2C 0x36 on Wire1 (internal bus, already begun by Device).
// I2S pins: BCK=34, WS=33, DOUT=13 on I2S_NUM_1.
//
// Tone uses a CoreS3-specific sine wave task (not SpeakerI2S square wave):
//   - i2s_set_clk() before writing ensures AW88298 I2S receiver locks to BCK.
//   - Sine wave LUT matches M5Unified _default_tone_wav exactly.
//   - _coreAmplitude mirrors SpeakerI2S::_amplitude (private there) via setVolume().
//

#pragma once

#include "core/SpeakerI2S.h"
#include "../lib/AW9523B.h"
#include <driver/i2s.h>
#include <Wire.h>
#include <math.h>

class SpeakerCoreS3 : public SpeakerI2S
{
public:
  explicit SpeakerCoreS3(AW9523B* aw) : _aw(aw) {}

  void begin() override {
    baseAmplitude = 3000;
    _aw->setSpeakerEnable(true);
    _aw88298Write(0x61, 0x0673);
    _aw88298Write(0x04, 0x4040);
    _aw88298Write(0x05, 0x0008);
    _aw88298Write(0x06, 0x14C7);
    _aw88298Write(0x0C, 0x0064);
    SpeakerI2S::begin();
  }

  // Mirror SpeakerI2S::_amplitude locally so _sineToneTask can read it.
  void setVolume(uint8_t vol) override {
    SpeakerI2S::setVolume(vol);
    _coreAmplitude = (int16_t)((uint32_t)vol * baseAmplitude / 100);
  }

  bool isPlaying() override {
    return _coreToneHandle != nullptr || SpeakerI2S::isPlaying();
  }

  // AW88298 is inaudible below ~700 Hz — octave-shift up, then use CoreS3 sine task.
  void tone(uint16_t freq, uint32_t durationMs) override {
    while (freq > 0 && freq < 700) freq <<= 1;
    _stopCoreTone();
    // Stop any base-class WAV/seq task so it cannot race with _sineToneTask on the
    // same I2S port.  _wavTask may still be running (e.g. WIN_SOUND started 300 ms
    // before the screen appears).  noTone() kills _taskHandle and zeros the DMA;
    // it does NOT call i2s_set_clk, so the I2S rate is left at the WAV's rate — a
    // genuine rate change awaits the sine task.
    SpeakerI2S::noTone();
    _coreFreq = freq;
    _coreDur  = durationMs;
    xTaskCreate(_sineToneTask, "spktone", 4096, this, 2, &_coreToneHandle);
  }

  void noTone() override {
    _stopCoreTone();
    SpeakerI2S::noTone();
  }

  // +18 semitones: all scale notes land in 740–1397 Hz, above AW88298 audible floor.
  void playRandomTone(int semitoneShift = 0, uint32_t durationMs = 150) override {
    static constexpr int scale[] = {60, 62, 64, 65, 67, 69, 71};
    int      midi = scale[random(0, 7)] + semitoneShift + 18;
    uint16_t freq = (uint16_t)(440.0f * powf(2.0f, (float)(midi - 69) / 12.0f));
    tone(freq, durationMs);
  }

private:
  AW9523B*     _aw;
  int16_t      _coreAmplitude = 16383;
  uint16_t     _coreFreq      = 1000;
  uint32_t     _coreDur       = 150;
  TaskHandle_t _coreToneHandle = nullptr;

  static constexpr uint8_t AW88298_ADDR = 0x36;

  void _stopCoreTone() {
    if (_coreToneHandle) {
      vTaskDelete(_coreToneHandle);
      _coreToneHandle = nullptr;
    }
  }

  static void _sineToneTask(void* arg) {
    auto* self = static_cast<SpeakerCoreS3*>(arg);
    // static constexpr local — avoids linker error from class-member constexpr arrays
    static constexpr uint8_t sineWav[16] = {
      177, 219, 246, 255, 246, 219, 177, 128, 79, 37, 10, 1, 10, 37, 79, 128
    };

    // AW88298 mutes when BCK glitches.  i2s_set_clk(same_rate) briefly drops BCK
    // even when the rate doesn't change (hardware re-arms the dividers).  WAV tasks
    // end with i2s_set_clk(44100 Hz) which mutes AW88298 — any same-rate call from
    // the tone task leaves it muted for the entire tone duration.
    //
    // Fix: use 22050 Hz as an intermediate, forcing two genuine BCK transitions
    // (prior_rate → 22050 → 44100).  AW88298 re-locks cleanly on each transition
    // regardless of what rate the I2S was left at by the previous task.
    i2s_set_clk((i2s_port_t)SPK_I2S_PORT, 22050, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
    i2s_set_clk((i2s_port_t)SPK_I2S_PORT, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);

    const uint32_t sampleRate  = 44100;
    uint32_t totalFrames = sampleRate * self->_coreDur / 1000;
    // Phase accumulator: bits[11:8] index into _sineWav[0..15], bits[7:0] sub-step.
    uint32_t phaseInc    = ((uint32_t)self->_coreFreq * 16u * 256u) / sampleRate;
    if (phaseInc == 0) phaseInc = 1;
    uint32_t phase = 0;

    int16_t  buf[128];
    uint32_t done = 0;

    while (done < totalFrames) {
      uint32_t chunk = (totalFrames - done) < 64 ? (totalFrames - done) : 64;
      for (uint32_t i = 0; i < chunk; i++) {
        uint8_t idx = (phase >> 8) & 0xF;
        int16_t s   = (int16_t)((int32_t)(sineWav[idx] - 128) * self->_coreAmplitude / 127);
        buf[i * 2]     = s;
        buf[i * 2 + 1] = s;
        phase += phaseInc;
      }
      size_t written;
      i2s_write((i2s_port_t)SPK_I2S_PORT, buf, chunk * 2 * sizeof(int16_t), &written, portMAX_DELAY);
      done += chunk;
    }
    i2s_zero_dma_buffer((i2s_port_t)SPK_I2S_PORT);
    self->_coreToneHandle = nullptr;
    vTaskDelete(nullptr);
  }

  void _aw88298Write(uint8_t reg, uint16_t val) {
    Wire1.beginTransmission(AW88298_ADDR);
    Wire1.write(reg);
    Wire1.write((uint8_t)(val >> 8));
    Wire1.write((uint8_t)(val & 0xFF));
    Wire1.endTransmission();
  }
};
