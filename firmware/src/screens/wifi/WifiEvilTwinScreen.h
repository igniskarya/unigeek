#pragma once

#include <esp_wifi.h>
#include "ui/templates/ListScreen.h"

class DNSServer;
class AsyncWebServer;

class WifiEvilTwinScreen : public ListScreen
{
public:
  const char* title() override { return "Evil Twin"; }
  bool inhibitPowerOff() override { return _state == STATE_RUNNING; }

  WifiEvilTwinScreen() {
    memset(_scanItems,  0, sizeof(_scanItems));
    memset(_scanLabels, 0, sizeof(_scanLabels));
    memset(_scanValues, 0, sizeof(_scanValues));
  }
  ~WifiEvilTwinScreen() override;

  void onInit() override;
  void onItemSelected(uint8_t index) override;
  void onUpdate() override;
  void onBack() override;

private:
  typedef uint8_t MacAddr[6];

  struct Target {
    String  ssid    = "-";
    MacAddr bssid   = {0, 0, 0, 0, 0, 0};
    int     channel = 1;
  };

  enum State { STATE_MENU, STATE_SELECT_WIFI, STATE_SELECT_PORTAL, STATE_RUNNING };

  State  _state   = STATE_MENU;
  Target _target;
  bool   _deauth  = false;
  String _portalFolder;  // empty = default

  class WifiAttackUtil* _attacker = nullptr;
  DNSServer*      _dns    = nullptr;
  AsyncWebServer* _server = nullptr;

  // Menu items
  ListItem _menuItems[5];
  String   _networkSub;
  String   _deauthSub;
  String   _portalSub;

  // Scan items
  static constexpr int MAX_SCAN = 20;
  ListItem _scanItems[MAX_SCAN];
  char     _scanLabels[MAX_SCAN][52];
  char     _scanValues[MAX_SCAN][18];
  int      _scanCount = 0;

  // Portal selection
  static constexpr int MAX_PORTALS = 10;
  ListItem _portalItems[MAX_PORTALS];
  String   _portalNames[MAX_PORTALS];
  int      _portalCount = 0;

  // Running state
  static constexpr int MAX_LOG = 20;
  char     _logLines[MAX_LOG][60];
  int      _logCount    = 0;
  int      _pwdCount    = 0;
  unsigned long _lastDeauth = 0;
  unsigned long _lastDraw   = 0;

  // Portal HTML
  String _portalHtml;
  String _successHtml;
  String _portalBasePath;  // e.g. "/unigeek/wifi/portals/google"

  void _showMenu();
  void _selectWifi();
  void _selectPortal();
  void _checkPassword();
  void _startAttack();
  void _stopAttack();
  void _addLog(const char* msg);
  void _drawLog();
  void _loadPortalHtml();
  void _saveCaptured(const String& email, const String& password);
  void _serveStaticFile(AsyncWebServerRequest* req, const String& path);
  String _getMimeType(const String& path);

  static const char* _defaultPortalHtml();
  static const char* _defaultSuccessHtml();
};
