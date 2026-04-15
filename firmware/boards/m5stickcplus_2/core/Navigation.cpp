//
// M5StickC Plus 2 navigation implementation.
// BTN_UP (GPIO 35) = UP, BTN_A (GPIO 37) = SELECT.
// BTN_B (GPIO 39): hold >600 ms = BACK, short press = DOWN.
//

#include "Navigation.h"
#include "pins_arduino.h"
#include <Arduino.h>

static constexpr uint32_t kLongPressMs = 600;

void NavigationImpl::begin() {
  pinMode(BTN_A,  INPUT);
  pinMode(BTN_UP, INPUT);
  pinMode(BTN_B,  INPUT);
}

void NavigationImpl::update() {
  uint32_t now = millis();

  // BTN_A → SELECT (takes priority)
  if (digitalRead(BTN_A) == LOW) {
    updateState(INavigation::DIR_PRESS);
    return;
  }

  static uint32_t btnBPressMs = 0;
  static bool     btnBLong    = false;
  bool btnBDown = (digitalRead(BTN_B) == LOW);

  if (btnBDown) {
    if (btnBPressMs == 0) btnBPressMs = now;
    if (!btnBLong && (now - btnBPressMs) > kLongPressMs) {
      btnBLong = true;
      updateState(INavigation::DIR_BACK);
      return;
    }
    updateState(INavigation::DIR_NONE);
    return;
  } else {
    if (btnBPressMs != 0 && !btnBLong) {
      btnBPressMs = 0;
      updateState(INavigation::DIR_DOWN);
      return;
    }
    btnBPressMs = 0;
    btnBLong    = false;
  }

  // BTN_UP → UP
  if (digitalRead(BTN_UP) == LOW) {
    updateState(INavigation::DIR_UP);
    return;
  }

  updateState(INavigation::DIR_NONE);
}
