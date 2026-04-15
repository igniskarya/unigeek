//
// CYD-2432W328R — XPT2046 resistive touch via TFT_eSPI getTouch().
//
// Touch zones (landscape 320×240, rotation=1):
//   Left 1/3  (x < 107):                BACK
//   Right 2/3 (x >= 107), top 1/3:      UP
//   Right 2/3 (x >= 107), middle 1/3:   SELECT (PRESS)
//   Right 2/3 (x >= 107), bottom 1/3:   DOWN
//
// drawOverlay() paints 2 px edge bars on left (BACK) and right (UP/SEL/DOWN),
// dim when idle, bright when the zone is held.
//

#pragma once

#include "core/INavigation.h"

class NavigationImpl : public INavigation
{
public:
  void begin()       override;
  void update()      override;
  void drawOverlay() override;

private:
  Direction _curDir     = DIR_NONE;
  uint8_t   _noTouchCnt = 0;
};
