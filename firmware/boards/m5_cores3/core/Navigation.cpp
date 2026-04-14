//
// M5Stack CoreS3 — touch-based navigation.
// FT6336U on CoreS3 reports in landscape coords directly (confirmed via M5GFX):
//   rawX: 0..319 (left → right),  rawY: 0..239 (top → bottom).
// No rotation transform needed — use rawX/rawY directly as screen X/Y.
//
// Touch zones (landscape 320×240):
//   Left 1/3  (x < 107):                 BACK
//   Right 2/3 (x >= 107), top 1/3:       UP
//   Right 2/3 (x >= 107), middle 1/3:    SELECT
//   Right 2/3 (x >= 107), bottom 1/3:    DOWN
//
// drawOverlay() is called by main.cpp AFTER Screen.update() so borders
// draw on top of all screen content and are never overwritten mid-frame.
//

#include "Navigation.h"
#include "pins_arduino.h"
#include "core/Device.h"
#include <Arduino.h>

static constexpr int16_t SCREEN_W = 320;
static constexpr int16_t SCREEN_H = 240;

// Zone boundaries
static constexpr int16_t BACK_END = SCREEN_W / 3;   // 107 — left 1/3 = BACK
static constexpr int16_t ZONE_H   = SCREEN_H / 3;   //  80 — right 2/3 split top-to-bottom

// Consecutive no-touch polls required to confirm a release (~60ms at 20ms poll rate)
static constexpr uint8_t NO_TOUCH_THRESHOLD = 3;

void NavigationImpl::begin() {
  touch.begin(Wire1);
}


void NavigationImpl::update() {
  static uint32_t lastPoll = 0;

  uint32_t now = millis();
  if (now - lastPoll < 20) {
    updateState(_curDir);
    return;
  }
  lastPoll = now;

  int16_t rawX, rawY;

  if (!touch.read(rawX, rawY)) {
    if (++_noTouchCnt < NO_TOUCH_THRESHOLD) {
      // Debouncing spurious no-touch — hold current direction
      updateState(_curDir);
      return;
    }
    // Confirmed release
    _curDir = DIR_NONE;
    updateState(DIR_NONE);
    return;
  }

  _noTouchCnt = 0;

  // FT6336U on CoreS3 reports landscape coords directly (confirmed via M5GFX source).
  // rawX: 0..319 left→right,  rawY: 0..239 top→bottom — use as-is.
  int16_t sx = rawX;
  int16_t sy = rawY;
  if (sx < 0) sx = 0; if (sx >= SCREEN_W) sx = SCREEN_W - 1;
  if (sy < 0) sy = 0; if (sy >= SCREEN_H) sy = SCREEN_H - 1;

  // Map to zone
  Direction dir;
  if (sx < BACK_END) {
    dir = DIR_BACK;
  } else {
    // Right 2/3: split top-to-bottom
    if      (sy < ZONE_H)      dir = DIR_UP;
    else if (sy < ZONE_H * 2)  dir = DIR_PRESS;
    else                       dir = DIR_DOWN;
  }

  _curDir = dir;
  updateState(_curDir);
}
