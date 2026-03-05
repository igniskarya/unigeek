//
// Created by L Shaf on 2026-02-23.
//

#pragma once

#include "core/INavigation.h"
#include "core/IKeyboard.h"
#include "pins_arduino.h"
#include <RotaryEncoder.h>

#define ROTARY_A  ENCODER_A
#define ROTARY_B  ENCODER_B
#define ROTARY_C  ENCODER_BTN

class NavigationImpl : public INavigation
{
public:
  NavigationImpl(IKeyboard* kb = nullptr) : _kb(kb) {}

  void begin() override;
  void update() override;

private:
  static constexpr unsigned long BTN_DEBOUNCE_MS = 150;
  static constexpr int           SCROLL_THRESH   = 2;

  RotaryEncoder* _encoder        = nullptr;
  IKeyboard*     _kb             = nullptr;
  int            _lastPos        = 0;
  int            _posDiff        = 0;

  // Button debounce
  bool          _btnRaw          = false;
  bool          _btnStable       = false;
  unsigned long _btnChangedAt    = 0;
};