//
// M5Stack CoreS3 — backlight via AXP2101 DLDO1.
//

#pragma once

#include "core/IDisplay.h"
#include "../lib/AXP2101.h"

class DisplayImpl : public IDisplay
{
public:
  DisplayImpl(AXP2101* axp) : _axp(axp) {}

  void setBrightness(uint8_t pct) override {
    _axp->setBacklight(pct);
  }

private:
  AXP2101* _axp;
};
