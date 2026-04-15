//
// CYD-2432W328R — XPT2046 resistive touch via TFT_eSPI getTouch().
//
// Default XPT2046 calibration for 2432W328R (rotation=1, landscape 320×240):
//   Raw ADC range: X 185–3700, Y 280–3850.
//   setTouch() maps these to screen pixels. Adjust if touch feels off.
//

#include "Navigation.h"
#include "core/Device.h"
#include <Arduino.h>

static constexpr int16_t SCREEN_W = 320;
static constexpr int16_t SCREEN_H = 240;
static constexpr int16_t BACK_END = SCREEN_W / 3;   // 107 — left 1/3 = BACK
static constexpr int16_t ZONE_H   = SCREEN_H / 3;   //  80 — right 2/3 split top-to-bottom

// Consecutive no-touch polls required to confirm release (~60 ms at 20 ms poll rate)
static constexpr uint8_t NO_TOUCH_THRESHOLD = 3;

void NavigationImpl::begin() {
  // Default XPT2046 calibration for CYD-2432W328R / 2432S024R (rotation=1).
  // Measured from Bruce firmware defaults: x_min=185, x_max=3700, y_min=280, y_max=3850.
  static uint16_t calData[5] = { 185, 3700, 280, 3850, 1 };
  Uni.Lcd.setTouch(calData);
}

void NavigationImpl::update() {
  static uint32_t lastPoll = 0;
  uint32_t now = millis();

  if (now - lastPoll < 20) {
    updateState(_curDir);
    return;
  }
  lastPoll = now;

  uint16_t tx, ty;
  bool touched = Uni.Lcd.getTouch(&tx, &ty);

  if (!touched) {
    if (++_noTouchCnt < NO_TOUCH_THRESHOLD) {
      updateState(_curDir);
      return;
    }
    _curDir = DIR_NONE;
    updateState(DIR_NONE);
    return;
  }

  _noTouchCnt = 0;

  // Clamp to screen bounds
  if (tx >= (uint16_t)SCREEN_W) tx = SCREEN_W - 1;
  if (ty >= (uint16_t)SCREEN_H) ty = SCREEN_H - 1;

  Direction dir;
  if (tx < (uint16_t)BACK_END) {
    dir = DIR_BACK;
  } else {
    if      (ty < (uint16_t)ZONE_H)      dir = DIR_UP;
    else if (ty < (uint16_t)(ZONE_H * 2)) dir = DIR_PRESS;
    else                                  dir = DIR_DOWN;
  }

  _curDir = dir;
  updateState(_curDir);
}

// Always-on 2 px edge bars mapping touch zones to screen edges:
//   BACK → left  edge (x=0..1), full height
//   UP   → right edge (x=318..319), top    third
//   SEL  → right edge (x=318..319), middle third
//   DOWN → right edge (x=318..319), bottom third
// Dim when idle, bright when that zone is held.
void NavigationImpl::drawOverlay() {
  static constexpr uint16_t DIM_RED   = 0x4000;
  static constexpr uint16_t LIT_RED   = 0xF800;
  static constexpr uint16_t DIM_GREEN = 0x0200;
  static constexpr uint16_t LIT_GREEN = 0x07E0;
  static constexpr uint16_t DIM_BLUE  = 0x0008;
  static constexpr uint16_t LIT_BLUE  = 0x001F;

  auto col = [&](Direction d, uint16_t dim, uint16_t lit) -> uint16_t {
    return (_curDir == d) ? lit : dim;
  };

  Uni.Lcd.fillRect(0,            0,              2, SCREEN_H,              col(DIR_BACK,  DIM_RED,   LIT_RED));
  Uni.Lcd.fillRect(SCREEN_W - 2, 0,              2, ZONE_H - 1,            col(DIR_UP,    DIM_GREEN, LIT_GREEN));
  Uni.Lcd.fillRect(SCREEN_W - 2, ZONE_H - 1,     2, 1,                     TFT_BLACK);
  Uni.Lcd.fillRect(SCREEN_W - 2, ZONE_H,         2, ZONE_H - 1,            col(DIR_PRESS, DIM_BLUE,  LIT_BLUE));
  Uni.Lcd.fillRect(SCREEN_W - 2, ZONE_H * 2 - 1, 2, 1,                     TFT_BLACK);
  Uni.Lcd.fillRect(SCREEN_W - 2, ZONE_H * 2,     2, SCREEN_H - ZONE_H * 2, col(DIR_DOWN,  DIM_GREEN, LIT_GREEN));
}
