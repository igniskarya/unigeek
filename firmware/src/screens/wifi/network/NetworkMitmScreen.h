#pragma once

#include "ui/templates/ListScreen.h"
#include "utils/DhcpStarvation.h"
#include "utils/RogueDhcpServer.h"
#include "utils/DnsSpoofServer.h"
#include "utils/WebFileManager.h"

class NetworkMitmScreen : public ListScreen {
public:
  const char* title()        override { return "MITM Attack"; }
  bool inhibitPowerSave()    override { return _state == STATE_RUNNING; }
  bool inhibitPowerOff()     override { return _state == STATE_RUNNING; }

  void onInit() override;
  void onUpdate() override;
  void onItemSelected(uint8_t index) override;
  void onBack() override;

private:
  enum State { STATE_MENU, STATE_RUNNING };
  State _state = STATE_MENU;

  // Options
  bool _rogueEnabled   = false;
  bool _dnsEnabled     = false;
  bool _fmEnabled      = false;
  bool _starvEnabled   = false;

  // Sublabels
  String _rogueSub;
  String _dnsSub;
  String _fmSub;
  String _starvSub;

  ListItem _menuItems[5];

  // Attack components
  DhcpStarvation  _starv;
  RogueDhcpServer _rogueDhcp;
  DnsSpoofServer  _dnsSpoof;
  WebFileManager  _fileManager;

  bool _starvRunning = false;

  // Log
  static constexpr int MAX_LOG = 30;
  char _logLines[MAX_LOG][60];
  int  _logCount = 0;
  unsigned long _lastDraw = 0;

  void _showMenu();
  void _start();
  void _stop();
  void _startRogueDhcp();
  void _addLog(const char* msg);
  void _drawLog();

  static NetworkMitmScreen* _instance;
  static void _onDnsVisit(const char* clientIP, const char* domain);
  static void _onDnsPost(const char* clientIP, const char* domain, const char* data);
  static void _onDhcpClient(const char* mac, const char* ip);
};
