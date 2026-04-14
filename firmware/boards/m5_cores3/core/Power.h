//
// M5Stack CoreS3 — AXP2101 power IC (I2C 0x34).
//

#pragma once

#include "core/IPower.h"
#include "../lib/AXP2101.h"

class PowerImpl : public IPower
{
public:
  PowerImpl(AXP2101* axp) : _axp(axp) {}

  void    begin()                override {}
  uint8_t getBatteryPercentage() override { return _axp->getBatteryLevel(); }
  void    powerOff()             override { _axp->powerOff(); }
  bool    isCharging()           override { return _axp->isCharging(); }

private:
  AXP2101* _axp;
};
