#include "ModuleMenuScreen.h"
#include "core/ScreenManager.h"
#include "screens/MainMenuScreen.h"
#include "screens/module/MFRC522Screen.h"
#include "screens/module/GPSScreen.h"
#include "screens/module/IRScreen.h"
#include "screens/module/SubGHzScreen.h"
#include "screens/module/NRF24Screen.h"
#include "screens/setting/PinSettingScreen.h"

void ModuleMenuScreen::onInit() {
  setItems(_items);
}

void ModuleMenuScreen::onBack() {
  Screen.setScreen(new MainMenuScreen());
}

void ModuleMenuScreen::onItemSelected(uint8_t index) {
  switch (index) {
    case 0: Screen.setScreen(new MFRC522Screen()); break;
    case 1: Screen.setScreen(new GPSScreen()); break;
    case 2: Screen.setScreen(new IRScreen()); break;
    case 3: Screen.setScreen(new SubGHzScreen()); break;
    case 4: Screen.setScreen(new NRF24Screen()); break;
    case 5: Screen.setScreen(new PinSettingScreen([]() -> IScreen* { return new ModuleMenuScreen(); })); break;
  }
}