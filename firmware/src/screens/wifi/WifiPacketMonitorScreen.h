#pragma once

#include <WiFi.h>
#include <esp_wifi.h>
#include <atomic>
#include <vector>

#include "ui/templates/BaseScreen.h"

class WifiPacketMonitorScreen : public BaseScreen
{
public:
  const char* title() override { return _title; }
  bool inhibitPowerSave() override { return true; }

  void onInit() override;
  void onUpdate() override;
  void onRender() override;

private:
  char     _title[24]    = "PM Channel 1";
  int8_t   _channel      = 1;
  uint32_t _lastRender   = 0;
  bool     _quitting     = false;

  std::vector<uint16_t> _history;

  void _setChannel(int8_t ch);
  void _quit();
};