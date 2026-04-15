//
// M5StickC Plus 2 — 3-button navigation (all GPIO, no AXP).
// BTN_UP (GPIO 35) = UP, BTN_A (GPIO 37) = SELECT.
// BTN_B (GPIO 39): short press = DOWN, long press (>600 ms) = BACK.
// ISR logic lives in Navigation.cpp (IRAM_ATTR requirement).
//

#pragma once

#include "core/INavigation.h"

class NavigationImpl : public INavigation
{
public:
  void begin() override;
  void update() override;
};
