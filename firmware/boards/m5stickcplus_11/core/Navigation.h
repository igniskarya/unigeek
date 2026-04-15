//
// Created by L Shaf on 2026-02-19.
//

#pragma once

#include "core/INavigation.h"
#include "lib/AXP192.h"

class NavigationImpl : public INavigation
{
public:
  NavigationImpl(AXP192* axp) : _axp(axp) {}
  void begin() override;
  void update() override;
private:
  AXP192* _axp;
};
