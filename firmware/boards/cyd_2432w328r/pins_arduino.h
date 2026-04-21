//
// CYD-2432W328R / 2432S024R / 2432S028 — ESP32, ILI9341 240×320, XPT2046 resistive touch.
//
// Display SPI (HSPI): MOSI=13, MISO=12, SCK=14, CS=15, DC=2
// Touch (XPT2046, HSPI shared): CS=33, IRQ=36
// SD card (VSPI default SPI):  CS=5,  SCK=18, MISO=19, MOSI=23
// Backlight: LCD_BL (LEDC PWM) — GPIO 27 on 2432W328R, GPIO 21 on 2432S028
//

#pragma once
#include <stdint.h>

// ─── SPI Bus (VSPI — SD card) ─────────────────────────────
#define SPI_SS_PIN    5
#define SPI_MOSI_PIN  23
#define SPI_MISO_PIN  19
#define SPI_SCK_PIN   18

static const uint8_t SS   = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t MISO = SPI_MISO_PIN;
static const uint8_t SCK  = SPI_SCK_PIN;

// ─── I2C ──────────────────────────────────────────────────
// Default: 2432W328R (SDA=21). Override via build flag for 2432S028 (SDA=27).
#ifndef GROVE_SDA
#define GROVE_SDA  21
#endif
#define GROVE_SCL  22

static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// ─── SD Card ──────────────────────────────────────────────
#define SD_CS  5

// ─── LCD Backlight ────────────────────────────────────────
// Default: 2432W328R (GPIO 27). Override via build flag for 2432S028 (GPIO 21).
#ifndef LCD_BL
#define LCD_BL  27
#endif
#define LCD_BL_CH  0

// ─── Touch Interrupt ──────────────────────────────────────
#define TOUCH_IRQ  36

// ─── IR Transmitter ──────────────────────────────────────
#define IR_TX_PIN  22

// ─── TFT_eSPI config ──────────────────────────────────────
#define DISABLE_ALL_LIBRARY_WARNINGS 1
#define USER_SETUP_LOADED 1

#define ILI9341_2_DRIVER        // ILI9341 v2
#define TFT_WIDTH   240
#define TFT_HEIGHT  320

#define USE_HSPI_PORT           // display on HSPI bus
#define TFT_MISO    12
#define TFT_MOSI    13
#define TFT_SCLK    14
#define TFT_CS      15
#define TFT_DC      2
#define TFT_RST     -1
#define TFT_BL      -1          // backlight via LEDC (see LCD_BL)
#define TFT_BACKLIGHT_ON HIGH
#define TFT_DEFAULT_ORIENTATION 1  // landscape 320×240

#define TOUCH_CS    33          // XPT2046 on same HSPI
#define SMOOTH_FONT

// Default: 2432W328R (55/20 MHz). Override via build flags for 2432S028 (40/16 MHz).
#ifndef SPI_FREQUENCY
#define SPI_FREQUENCY        55000000
#endif
#ifndef SPI_READ_FREQUENCY
#define SPI_READ_FREQUENCY   20000000
#endif
#define SPI_TOUCH_FREQUENCY  2500000

// ─── Firmware Feature Flags ───────────────────────────────
#define DEVICE_HAS_TOUCH_NAV   // XPT2046 resistive touch, no physical buttons
