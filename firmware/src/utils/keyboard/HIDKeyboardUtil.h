#pragma once

#include <stdint.h>
#include <string>
#include "Print.h"

// ── Key code offsets ────────────────────────────────────────────────────────
#define HID_OFFSET       0x88
#define KEY_LEFT_CTRL    0x80
#define KEY_LEFT_SHIFT   0x81
#define KEY_LEFT_ALT     0x82
#define KEY_LEFT_GUI     0x83
#define KEY_RIGHT_CTRL   0x84
#define KEY_RIGHT_SHIFT  0x85
#define KEY_RIGHT_ALT    0x86
#define KEY_RIGHT_GUI    0x87

#define KEY_UP_ARROW     0xDA
#define KEY_DOWN_ARROW   0xD9
#define KEY_LEFT_ARROW   0xD8
#define KEY_RIGHT_ARROW  0xD7
#define KEY_BACKSPACE    0xB2
#define KEY_TAB          0xB3
#define KEY_RETURN       0xB0
#define KEY_ESC          0xB1
#define KEY_INSERT       0xD1
#define KEY_DELETE       0xD4
#define KEY_PAGE_UP      0xD3
#define KEY_PAGE_DOWN    0xD6
#define KEY_HOME         0xD2
#define KEY_END          0xD5
#define KEY_CAPS_LOCK    0xC1
#define KEY_F1           0xC2
#define KEY_F2           0xC3
#define KEY_F3           0xC4
#define KEY_F4           0xC5
#define KEY_F5           0xC6
#define KEY_F6           0xC7
#define KEY_F7           0xC8
#define KEY_F8           0xC9
#define KEY_F9           0xCA
#define KEY_F10          0xCB
#define KEY_F11          0xCC
#define KEY_F12          0xCD

typedef struct {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} KeyReport;

// Standard HID keyboard report descriptor (no TinyUSB dependency)
static const uint8_t kHIDReportDescriptor[] = {
  0x05, 0x01,  // Usage Page (Generic Desktop)
  0x09, 0x06,  // Usage (Keyboard)
  0xA1, 0x01,  // Collection (Application)
  0x85, 0x01,  //   Report ID (1)
  0x05, 0x07,  //   Usage Page (Keyboard)
  0x19, 0xE0,  //   Usage Minimum (Left Ctrl)
  0x29, 0xE7,  //   Usage Maximum (Right GUI)
  0x15, 0x00,  //   Logical Minimum (0)
  0x25, 0x01,  //   Logical Maximum (1)
  0x75, 0x01,  //   Report Size (1)
  0x95, 0x08,  //   Report Count (8)
  0x81, 0x02,  //   Input (Data, Var, Abs) — modifiers
  0x95, 0x01,  //   Report Count (1)
  0x75, 0x08,  //   Report Size (8)
  0x81, 0x01,  //   Input (Const) — reserved
  0x95, 0x05,  //   Report Count (5)
  0x75, 0x01,  //   Report Size (1)
  0x05, 0x08,  //   Usage Page (LEDs)
  0x19, 0x01,  //   Usage Minimum (Num Lock)
  0x29, 0x05,  //   Usage Maximum (Kana)
  0x91, 0x02,  //   Output (Data, Var, Abs)
  0x95, 0x01,  //   Report Count (1)
  0x75, 0x03,  //   Report Size (3)
  0x91, 0x01,  //   Output (Const)
  0x95, 0x06,  //   Report Count (6)
  0x75, 0x08,  //   Report Size (8)
  0x15, 0x00,  //   Logical Minimum (0)
  0x25, 0x65,  //   Logical Maximum (101)
  0x05, 0x07,  //   Usage Page (Keyboard)
  0x19, 0x00,  //   Usage Minimum (Reserved)
  0x29, 0x65,  //   Usage Maximum (Application)
  0x81, 0x00,  //   Input (Data, Array, Abs) — keys
  0xC0,        // End Collection
};

class HIDKeyboardUtil : public Print {
protected:
  uint32_t  _delayMs = 8;
  KeyReport _keyReport = {};

  uint16_t _vid     = 0xe502;
  uint16_t _pid     = 0xa111;
  uint16_t _version = 0x0210;
  std::string _manufacturer;

public:
  ~HIDKeyboardUtil() override = default;

  virtual void begin()                      = 0;
  virtual void end()                        = 0;
  virtual void sendReport(KeyReport* keys)  = 0;
  virtual bool isConnected()                { return true; }
  virtual void setBatteryLevel(uint8_t)     {}
  virtual void resetPair()                  {}

  void setDelayMs(uint32_t ms)              { _delayMs = ms; }

  void reportModifier(KeyReport* report, uint8_t key);
  size_t write(uint8_t k)                   override;
  size_t write(const uint8_t* buf, size_t size) override;
  size_t press(uint8_t k);
  size_t release(uint8_t k);
  void   releaseAll();
};
