#include "WifiPacketMonitorScreen.h"
#include "core/Device.h"
#include "core/ScreenManager.h"
#include "core/ConfigManager.h"
#include "screens/wifi/WifiMenuScreen.h"

// ── Promiscuous callback ───────────────────────────────────────────────────

static std::atomic<uint32_t> _pktCounter{0};
static std::atomic<uint32_t> _cbActive{0};

static void _snifferCb(void* buf, wifi_promiscuous_pkt_type_t type)
{
  if (type == WIFI_PKT_MGMT || type == WIFI_PKT_DATA) {
    _pktCounter.fetch_add(1, std::memory_order_relaxed);
  }
  _cbActive.fetch_sub(1, std::memory_order_release);
}

// ── Lifecycle ─────────────────────────────────────────────────────────────

void WifiPacketMonitorScreen::onInit()
{
  _history.assign(bodyW() / 4, 0);
  _quitting = false;

  WiFi.mode(WIFI_MODE_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&_snifferCb);
  _setChannel(1);
}

void WifiPacketMonitorScreen::onUpdate()
{
  if (_quitting) return;

  if (Uni.Nav->wasPressed()) {
    const auto dir = Uni.Nav->readDirection();
    if (dir == INavigation::DIR_BACK) {
      _quit();
      return;
    }
    if (dir == INavigation::DIR_UP)   { _setChannel(_channel < 13 ? _channel + 1 : 1);  }
    if (dir == INavigation::DIR_DOWN) { _setChannel(_channel > 1  ? _channel - 1 : 13); }
  }

  if (millis() - _lastRender >= 1000) {
    render();
  }
}

void WifiPacketMonitorScreen::onRender()
{
  if (_quitting) return;

  _lastRender = millis();

  const uint32_t rawCount = _pktCounter.exchange(0, std::memory_order_relaxed);
  auto sample = static_cast<uint16_t>(rawCount > 180 ? 90 : (rawCount + 1) / 2);

  // shift history left, append new sample
  const int histSize = static_cast<int>(_history.size());
  for (int i = histSize - 1; i > 0; --i) _history[i] = _history[i - 1];
  _history[0] = sample;

  const uint16_t themeColor = Config.getThemeColor();

  TFT_eSprite body(&Uni.Lcd);
  body.createSprite(bodyW(), bodyH());
  body.fillSprite(TFT_BLACK);

  const uint16_t barW   = 3;
  const uint16_t barGap = 1;
  const uint16_t maxH   = bodyH() - body.fontHeight() - 12;

  for (int i = 0; i < histSize; i++) {
    uint16_t h = _history[i];
    if (h > maxH) h = maxH;
    uint16_t x = i * (barW + barGap);
    body.fillRect(x, bodyH() - body.fontHeight() - 10 - h, barW, h, themeColor);
  }

  body.setTextColor(TFT_WHITE, TFT_BLACK);
  body.drawString(String(rawCount) + " pkt/s", 0, 0);
  body.drawString("UP/DN: Channel | BACK: Exit", 0, bodyH() - body.fontHeight());

  body.pushSprite(bodyX(), bodyY());
  body.deleteSprite();
}

// ── Private ───────────────────────────────────────────────────────────────

void WifiPacketMonitorScreen::_setChannel(int8_t ch)
{
  _channel = ch;
  snprintf(_title, sizeof(_title), "PM Channel %d", _channel);
  esp_wifi_set_channel(_channel, WIFI_SECOND_CHAN_NONE);
  render();
}

void WifiPacketMonitorScreen::_quit()
{
  _quitting = true;

  esp_wifi_set_promiscuous_rx_cb(nullptr);

  uint32_t waited = 0;
  while (_cbActive.load(std::memory_order_acquire) != 0 && waited < 200) {
    delay(5);
    waited += 5;
  }

  esp_wifi_set_promiscuous(false);

  Screen.setScreen(new WifiMenuScreen());
}