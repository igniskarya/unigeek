//
// Created by L Shaf on 2026-02-23.
//

#pragma once

#include "core/Device.h"
#include "core/ConfigManager.h"

class InputNumberAction
{
public:
  static int popup(const char* title, int min = INT_MIN, int max = INT_MAX, int defaultValue = 0) {
    InputNumberAction action(title, min, max, defaultValue);
    int result = action._run();
    _lastCancelled = action._cancelled;
    Uni.lastActiveMs = millis();
    return result;
  }

  static bool wasCancelled() { return _lastCancelled; }

private:
  static constexpr uint32_t BLINK_MS = 500;
  static constexpr int      PAD      = 4;
  static constexpr int      INP_H    = 16;
  static constexpr int      ROW_H    = 18;
  static constexpr int      ERR_H    = 10;
  static constexpr int      RANGE_H  = 10;

  const char* _title;
  int         _minValidator;
  int         _maxValidator;
  bool        _hasMin;
  bool        _hasMax;

  String      _input;
  String      _error;
  String      _lastError;

  bool        _done        = false;
  bool        _cancelled   = false;
  bool        _cursorVisible  = true;
  uint32_t    _lastBlinkTime  = 0;

  inline static bool _lastCancelled = false;

  static constexpr int DIGIT_COUNT = 12;
  int         _scrollPos   = 0;

  struct DigitSet {
    const char* label;
    bool        isAction;
    enum Action { ACT_DEL, ACT_SAVE, ACT_CANCEL } action;
  };

  DigitSet _sets[13];
  int      _setCount = 0;

  explicit InputNumberAction(const char* title, int min, int max, int defaultValue)
  : _title(title),
    _minValidator(min),
    _maxValidator(max),
    _hasMin(min != INT_MIN), _hasMax(max != INT_MAX),
    _input(defaultValue != 0 ? String(defaultValue) : "")
  {}

  void _buildSets() {
    static constexpr const char* digits[] = {
      "0","1","2","3","4","5","6","7","8","9"
    };
    _setCount = 0;
    for (int i = 0; i < 10; i++) {
      _sets[_setCount++] = { digits[i], false, DigitSet::ACT_DEL };
    }
    _sets[_setCount++] = { "DEL",    true, DigitSet::ACT_DEL    };
    _sets[_setCount++] = { "SAVE",   true, DigitSet::ACT_SAVE   };
    _sets[_setCount++] = { "CANCEL", true, DigitSet::ACT_CANCEL };
  }

  bool _validate() {
    if (_input.length() == 0) {
      _error = "Please enter a number";
      return false;
    }
    int val = _input.toInt();
    if (_hasMin && val < _minValidator) {
      _error = "Min value: " + String(_minValidator);
      return false;
    }
    if (_hasMax && val > _maxValidator) {
      _error = "Max value: " + String(_maxValidator);
      return false;
    }
    _error = "";
    return true;
  }

  int _run() {
#ifdef DEVICE_HAS_KEYBOARD
    return _runKeyboard();
#else
    return _runScroll();
#endif
  }

  int _overlayH() {
    int h = 12 + 22 + 10 + 22 + 10 + 16 + PAD * 4;
    if (!_hasMin && !_hasMax) h -= 10;
    return h;
  }
  int _overlayW() { return Uni.Lcd.width() - (PAD * 2 + 8); }
  int _overlayX() { return PAD + 4; }
  int _overlayY() { return (Uni.Lcd.height() - _overlayH()) / 2; }

  int _inputY()  { return PAD + 12; }
  int _rangeY()  { return _inputY() + INP_H + PAD; }
  int _rowY()    { return _rangeY() + (_hasMin || _hasMax ? RANGE_H : 0) + PAD; }
  int _errorY()  { return _rowY()   + ROW_H + PAD; }
  int _hintY()   { return _overlayH() - PAD - 8; }

  // ── keyboard mode ──────────────────────────────────────
  int _runKeyboard() {
    _drawChrome();
    _drawInput(true);
    if (_error.length() > 0) _drawError();
    _lastError = _error;
    uint32_t lastBlink = millis();
    bool cursorOn = true;

    while (!_done && !_cancelled) {
      Uni.update();

      if (millis() - lastBlink >= BLINK_MS) {
        cursorOn  = !cursorOn;
        lastBlink = millis();
        _drawInput(cursorOn);
      }

      if (Uni.Keyboard && Uni.Keyboard->available()) {
        char c = Uni.Keyboard->getKey();
        String prevErr = _error;
        _error = "";

#ifdef KB_QWERT_NUM_REMAP
        static constexpr char topRow[]   = "qwertyuiop";
        static constexpr char topRowUp[] = "QWERTYUIOP";
        static constexpr char topNums[]  = "1234567890";
        for (int i = 0; i < 10; i++) {
          if (c == topRow[i] || c == topRowUp[i]) { c = topNums[i]; break; }
        }
#endif

        if (c == '\n') {
          if (_validate()) _done = true;
          else { _drawError(); }
        } else if (isdigit(c)) {
          _input += c;
          cursorOn  = true;
          lastBlink = millis();
          _drawInput(true);
        }
        if (prevErr != _error) _drawError();
      }

      if (Uni.Nav->wasPressed()) {
        auto dir = Uni.Nav->readDirection();
        if (dir == INavigation::DIR_PRESS) {
          String prevErr = _error;
          if (_validate()) _done = true;
          if (prevErr != _error) _drawError();
        } else if (dir == INavigation::DIR_BACK) {
          if (_input.length() > 0) {
            _input.remove(_input.length() - 1);
            cursorOn  = true;
            lastBlink = millis();
            _drawInput(true);
          } else {
            _cancelled = true;
          }
        }
      }
      delay(10);
    }

    Uni.Lcd.fillRect(_overlayX(), _overlayY(), _overlayW(), _overlayH(), TFT_BLACK);
    return _cancelled ? 0 : _input.toInt();
  }

  // ── scroll mode ────────────────────────────────────────
  int _runScroll() {
    _buildSets();
    _lastBlinkTime = millis();
    _cursorVisible = true;
    _drawChrome();
    _drawInput(true);
    _drawRow();
    _lastError = _error;

    while (!_done && !_cancelled) {
      Uni.update();

      if (millis() - _lastBlinkTime >= BLINK_MS) {
        _cursorVisible = !_cursorVisible;
        _lastBlinkTime = millis();
        _drawInput(_cursorVisible);
      }

      if (Uni.Nav->wasPressed()) {
        switch (Uni.Nav->readDirection()) {
          case INavigation::DIR_UP: {
            String prevErr = _error;
            _scrollPos     = (_scrollPos - 1 + _setCount) % _setCount;
            _error         = "";
            _cursorVisible = true;
            _lastBlinkTime = millis();
            _drawRow();
            if (prevErr != _error) _drawError();
            break;
          }

          case INavigation::DIR_DOWN: {
            String prevErr = _error;
            _scrollPos     = (_scrollPos + 1) % _setCount;
            _error         = "";
            _cursorVisible = true;
            _lastBlinkTime = millis();
            _drawRow();
            if (prevErr != _error) _drawError();
            break;
          }

          case INavigation::DIR_PRESS: {
            String prevErr = _error;
            _handleSelect();
            if (!_done && !_cancelled) {
              _cursorVisible = true;
              _lastBlinkTime = millis();
              _drawInput(true);
              if (prevErr != _error) _drawError();
            }
            break;
          }

          case INavigation::DIR_BACK:
            _cancelled = true;
            break;

          default: break;
        }
      }
      delay(10);
    }

    Uni.Lcd.fillRect(_overlayX(), _overlayY(), _overlayW(), _overlayH(), TFT_BLACK);
    return _cancelled ? 0 : _input.toInt();
  }

  void _handleSelect() {
    const DigitSet& s = _sets[_scrollPos];
    _error = "";

    if (s.isAction) {
      switch (s.action) {
        case DigitSet::ACT_SAVE:
          if (_validate()) _done = true;
          break;
        case DigitSet::ACT_DEL:
          if (_input.length() > 0)
            _input.remove(_input.length() - 1);
          break;
        case DigitSet::ACT_CANCEL:
          _cancelled = true;
          break;
      }
    } else {
      _input += s.label;
    }
  }

  void _drawChrome() {
    auto& lcd = Uni.Lcd;
    int w = _overlayW();
    int h = _overlayH();
    int x = _overlayX();
    int y = _overlayY();

    lcd.fillRect(x, y, w, h, TFT_BLACK);
    lcd.drawRoundRect(x, y, w, h, 4, TFT_WHITE);

    lcd.setTextColor(TFT_YELLOW);
    lcd.setTextSize(1);
    lcd.setTextDatum(TL_DATUM);
    lcd.setCursor(x + PAD, y + PAD);
    lcd.print(_title);

    if (_hasMin || _hasMax) {
      lcd.setTextColor(TFT_DARKGREY);
      lcd.setCursor(x + PAD, y + _rangeY());
      String rangeStr = "";
      if (_hasMin && _hasMax) rangeStr = "Range: " + String(_minValidator) + " - " + String(_maxValidator);
      else if (_hasMin)       rangeStr = "Min: " + String(_minValidator);
      else                    rangeStr = "Max: " + String(_maxValidator);
      lcd.print(rangeStr);
    }

    lcd.setTextColor(TFT_DARKGREY);
    lcd.setCursor(x + PAD, y + _hintY());
#ifdef DEVICE_HAS_KEYBOARD
    lcd.print("Numbers only + ENTER to confirm");
#else
    lcd.print("UP/DN:digit  PRESS:select");
#endif
  }

  void _drawInput(bool cursorOn) {
    auto& lcd  = Uni.Lcd;
    int w      = _overlayW();
    int x      = _overlayX();
    int y      = _overlayY();
    int innerW = w - PAD * 2;

    Sprite sp(&lcd);
    sp.createSprite(innerW, INP_H);
    sp.fillSprite(TFT_BLACK);
    sp.drawRoundRect(0, 0, innerW, INP_H, 3, TFT_DARKGREY);
    sp.setTextColor(TFT_WHITE);
    sp.setTextDatum(TL_DATUM);
    String display = _input;
    if (cursorOn) display += '_';
    sp.drawString(display.length() > 0 ? display.c_str() : (cursorOn ? "_" : " "), 3, 4);
    sp.pushSprite(x + PAD, y + _inputY());
    sp.deleteSprite();
  }

  void _drawRow() {
    auto& lcd = Uni.Lcd;
    uint16_t themeColor = Config.getThemeColor();
    int w      = _overlayW();
    int x      = _overlayX();
    int y      = _overlayY();
    int innerW = w - PAD * 2;
    int midX   = innerW / 2;

    int prev = (_scrollPos - 1 + _setCount) % _setCount;
    int curr =  _scrollPos;
    int next = (_scrollPos + 1) % _setCount;

    const char* prevLabel = _sets[prev].label;
    const char* currLabel = _sets[curr].label;
    const char* nextLabel = _sets[next].label;

    int currW = max(20, (int)(strlen(currLabel) * 6) + 10);

    Sprite sp(&lcd);
    sp.createSprite(innerW, ROW_H);
    sp.fillSprite(TFT_BLACK);
    sp.setTextSize(1);
    sp.setTextDatum(MC_DATUM);

    sp.setTextColor(TFT_DARKGREY);
    sp.drawString(prevLabel, midX - currW / 2 - 24, ROW_H / 2);

    sp.fillRoundRect(midX - currW / 2, 0, currW, ROW_H, 3, themeColor);
    sp.drawRoundRect(midX - currW / 2, 0, currW, ROW_H, 3, TFT_WHITE);
    sp.setTextColor(TFT_WHITE);
    sp.drawString(currLabel, midX, ROW_H / 2);

    sp.setTextColor(TFT_DARKGREY);
    sp.drawString(nextLabel, midX + currW / 2 + 24, ROW_H / 2);

    sp.pushSprite(x + PAD, y + _rowY());
    sp.deleteSprite();
  }

  void _drawError() {
    auto& lcd = Uni.Lcd;
    int w      = _overlayW();
    int x      = _overlayX();
    int y      = _overlayY();
    int innerW = w - PAD * 2;

    Sprite sp(&lcd);
    sp.createSprite(innerW, ERR_H);
    sp.fillSprite(TFT_BLACK);
    sp.setTextSize(1);
    sp.setTextDatum(TL_DATUM);
    if (_error.length() > 0) {
      sp.setTextColor(TFT_RED);
      sp.drawString(_error.c_str(), 0, 0);
    }
    sp.pushSprite(x + PAD, y + _errorY());
    sp.deleteSprite();
    _lastError = _error;
  }
};
