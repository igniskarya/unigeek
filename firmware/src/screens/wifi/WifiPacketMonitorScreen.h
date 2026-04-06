#pragma once

#include <WiFi.h>
#include <esp_wifi.h>
#include <atomic>

#include "ui/templates/BaseScreen.h"

class WifiPacketMonitorScreen : public BaseScreen
{
public:
  const char* title() override { return "Packet Monitor"; }
  bool inhibitPowerSave() override { return true; }

  void onInit() override;
  void onUpdate() override;
  void onRender() override;

private:
  static constexpr uint8_t  _NUM_CH  = 13;
  static constexpr uint32_t _HOP_MS  = 2000 / _NUM_CH;  // ~154 ms per channel

  uint8_t  _sweepCh         = 1;
  uint32_t _lastHop         = 0;
  bool     _quitting        = false;
  bool     _firstSweep      = true;

  uint32_t _chanAccum[_NUM_CH + 1]     = {};  // index 1–13, accumulates during sweep
  uint32_t _displayCounts[_NUM_CH + 1] = {};  // index 1–13, shown after sweep completes

  void _hop();
  void _quit();
};