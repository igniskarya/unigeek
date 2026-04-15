//
// CYD-2432W328R / 2432S024R — Device factory.
//
// Display:  ILI9341 on HSPI (MOSI=13, MISO=12, SCK=14, CS=15, DC=2), BL=27
// Touch:    XPT2046 on same HSPI (CS=33, IRQ=36) via TFT_eSPI getTouch()
// SD card:  VSPI default SPI (SCK=18, MISO=19, MOSI=23, CS=5)
// LittleFS: fallback when SD is absent
//
// SPI busses:
//   HSPI — display + touch (TFT_eSPI handles this internally with USE_HSPI_PORT)
//   VSPI — SD card (Arduino default SPI, init by initStorage() via SPI_SCK/MISO/MOSI pins)
//

#include "core/Device.h"
#include "Display.h"
#include "Power.h"
#include "Navigation.h"

static DisplayImpl    display;
static NavigationImpl navigation;
static PowerImpl      power;

void Device::applyNavMode() {}
void Device::boardHook() {}

Device* Device::createInstance() {
  // Enable backlight before display init so screen illuminates immediately.
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);

  // Pull SD CS high so it stays idle while TFT_eSPI inits the HSPI display.
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  // ExI2C available for Grove port (SDA=21, SCL=22)
  auto* dev = new Device(display, power, &navigation);
  dev->ExI2C = &Wire;
  return dev;
}
