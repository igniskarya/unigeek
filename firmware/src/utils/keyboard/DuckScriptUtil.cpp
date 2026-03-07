#include "DuckScriptUtil.h"

uint8_t DuckScriptUtil::_charToHID(const char* str)
{
  uint8_t value = 0;
  size_t  len   = strlen(str);
  if (len == 1) {
    value = (uint8_t)str[0];
  } else if (len == 2 && (str[0] == 'F' || str[0] == 'f')) {
    int fn = atoi(&str[1]);
    if (fn >= 1 && fn <= 12) value = (uint8_t)(0xC1 + fn);
  }
  return value;
}

void DuckScriptUtil::_holdPress(uint8_t modifier, uint8_t key)
{
  KeyReport r = {};
  _keyboard->reportModifier(&r, modifier);
  _keyboard->reportModifier(&r, key);
  _keyboard->sendReport(&r);
  delay(50);
  _keyboard->releaseAll();
}

bool DuckScriptUtil::runCommand(const String& line)
{
  if (line.startsWith("STRING ")) {
    String s = line.substring(7);
    _keyboard->write(reinterpret_cast<const uint8_t*>(s.c_str()), s.length());
  } else if (line.startsWith("STRINGLN ")) {
    String s = line.substring(9);
    _keyboard->write(reinterpret_cast<const uint8_t*>(s.c_str()), s.length());
    _keyboard->write(KEY_RETURN);
  } else if (line.startsWith("DELAY ")) {
    delay(line.substring(6).toInt());
  } else if (line.startsWith("GUI ")) {
    _holdPress(KEY_LEFT_GUI, _charToHID(line.substring(4).c_str()));
  } else if (line.startsWith("CTRL ")) {
    _holdPress(KEY_LEFT_CTRL, _charToHID(line.substring(5).c_str()));
  } else if (line.startsWith("ALT ")) {
    _holdPress(KEY_LEFT_ALT, _charToHID(line.substring(4).c_str()));
  } else if (line.startsWith("SHIFT ")) {
    _holdPress(KEY_LEFT_SHIFT, _charToHID(line.substring(6).c_str()));
  } else if (line.startsWith("ENTER")) {
    _keyboard->write(KEY_RETURN);
  } else if (line.startsWith("REM ") || line.startsWith("REM\t")) {
    // comment — do nothing
  } else {
    return false;
  }
  return true;
}