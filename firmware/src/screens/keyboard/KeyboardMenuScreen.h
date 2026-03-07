#pragma once

#include "ui/templates/ListScreen.h"

class KeyboardMenuScreen : public ListScreen {
public:
  const char* title() override { return "Keyboard"; }

  void onInit()             override;
  void onItemSelected(uint8_t index) override;
  void onBack()             override;

private:
#ifdef DEVICE_HAS_USB_HID
  bool _usbMode = true;

  String   _modeSub;
  ListItem _items[2] = {
    {"Mode",  nullptr},
    {"Start", nullptr},
  };

  void _refreshMode();
#else
  ListItem _items[1] = {
    {"Start"},
  };
#endif
};