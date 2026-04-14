//
// M5Stack CoreS3 — touch-based navigation (0 physical buttons).
// Screen split into touch zones (landscape 320×240, left-to-right):
//   x < 107         (left 1/3):   BACK
//   107 <= x < 178  (mid-left):   UP
//   178 <= x < 249  (mid-right):  SELECT
//   x >= 249        (right):      DOWN
//
// Zone indicator drawn in bottom 4px strip (y=236..239), persists across renders.
//

#pragma once

#include "core/INavigation.h"
#include "../lib/TouchFT6336U.h"

class NavigationImpl : public INavigation
{
public:
  void begin()  override;
  void update() override;

  TouchFT6336U touch;

private:
  Direction _curDir    = DIR_NONE;
  uint8_t   _noTouchCnt = 0;
};
