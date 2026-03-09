#pragma once

#include "core/INavigation.h"
#include "pins_arduino.h"
#include <Arduino.h>

class NavigationImpl : public INavigation
{
public:
  void begin() override {
    pinMode(UP_BTN, INPUT_PULLUP);
    pinMode(DW_BTN, INPUT_PULLUP);
  }

  void update() override
  {
    bool btnUp = (digitalRead(UP_BTN) == BTN_ACT);
    bool btnDown = (digitalRead(DW_BTN) == BTN_ACT);

    if (btnUp && btnDown) updateState(DIR_PRESS);
    else if (btnUp) updateState(DIR_UP);
    else if (btnDown) updateState(DIR_DOWN);
    else updateState(DIR_NONE);
  }
};
