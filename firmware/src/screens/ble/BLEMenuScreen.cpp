#include "BLEMenuScreen.h"
#include "core/Device.h"
#include "core/ScreenManager.h"
#include "screens/MainMenuScreen.h"
#include "screens/ble/BLEAnalyzerScreen.h"
#include "screens/ble/BLESpamScreen.h"
#include "screens/ble/BLEDetectorScreen.h"

void BLEMenuScreen::onInit()
{
  setItems(_items);
}

void BLEMenuScreen::onItemSelected(uint8_t index)
{
  switch (index) {
    case 0: Screen.setScreen(new BLEAnalyzerScreen());  break;
    case 1: Screen.setScreen(new BLESpamScreen());     break;
    case 2: Screen.setScreen(new BLEDetectorScreen()); break;
  }
}

void BLEMenuScreen::onBack()
{
  Screen.setScreen(new MainMenuScreen());
}