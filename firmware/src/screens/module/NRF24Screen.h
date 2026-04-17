//
// NRF24L01+ module screen — spectrum, jammer, MouseJack
// Reference: Bruce firmware NRF24 module
//

#pragma once
#include "ui/templates/ListScreen.h"
#include "utils/nrf24/NRF24Util.h"

class NRF24Screen : public ListScreen {
public:
  const char* title() override { return _titleBuf; }
  bool inhibitPowerSave() override;
  bool inhibitPowerOff() override;

  void onInit() override;
  void onUpdate() override;
  void onRender() override;
  void onBack() override;
  void onItemSelected(uint8_t index) override;

  // HID injection
  struct HidKey { uint8_t mod; uint8_t key; };
  static bool _asciiToHid(char c, HidKey& out);

private:
  enum State {
    STATE_MENU,
    STATE_SPECTRUM,
    STATE_JAMMER_MENU,
    STATE_JAMMER_RUNNING,
    STATE_CH_JAMMER,
    STATE_HOPPER_RUN,
    STATE_MJ_SCAN,
  } _state = STATE_MENU;

  NRF24Util _nrf;
  int8_t _cePin  = -1;
  int8_t _csnPin = -1;
  char   _titleBuf[22] = "NRF24L01";
  bool   _chromeDrawn  = false;

  // ─── Main menu ──────────────────────────────────────────────────
  ListItem _mainItems[3] = {{"Spectrum"}, {"Jammer"}, {"MouseJack"}};

  // ─── Spectrum ────────────────────────────────────────────────────
  static constexpr int kSpecCh         = 126;
  static constexpr int kPeakHoldSweeps = 25;
  uint8_t _specCh[kSpecCh]    = {};
  uint8_t _specPeak[kSpecCh]  = {};
  uint8_t _specPeakT[kSpecCh] = {};
  uint8_t _specMode   = 0;   // 0=peaks, 1=bars
  int     _specScanCh = 0;   // incremental scan position

  // ─── Jammer ──────────────────────────────────────────────────────
  static constexpr int kJamModes = 10;
  ListItem _jamMenuItems[12];  // 10 presets + Single CH + CH Hopper
  int      _jamMode   = 0;     // active preset mode (0–9)
  int      _jamHopIdx = 0;     // channel hop index within mode list
  uint32_t _lastHopMs  = 0;
  uint32_t _lastRender = 0;

  // Single-channel jammer
  int  _chJamCh     = 50;
  bool _chJamPaused = false;

  // Channel hopper
  int _hopStart = 0;
  int _hopStop  = 80;
  int _hopStep  = 2;
  int _hopCh    = 0;

  // ─── MouseJack ───────────────────────────────────────────────────
  static constexpr int kMjMax = 12;
  struct MjTarget {
    uint8_t addr[5];
    uint8_t addrLen;
    uint8_t ch;
    uint8_t type;   // 0=unknown, 1=ms, 2=ms-crypt, 3=logitech
    bool    active;
  };
  MjTarget _mjTargets[kMjMax] = {};
  uint8_t  _mjCount    = 0;
  int      _mjSelected = 0;
  int      _mjScanCh   = 0;
  uint16_t _mjMsSeq    = 0;
  char     _mjAddrBufs[kMjMax][18];

  // ─── Helpers ─────────────────────────────────────────────────────
  void _showMenu();
  void _showJammerMenu();
  bool _radioBegin();
  void _radioEnd();

  // Spectrum
  void     _scanSpectrum();
  void     _renderSpectrum();
  uint16_t _specBarColor(uint8_t level);

  // Jammer
  void _initCW(int ch);
  void _jamStep();
  void _renderJammerStatus();
  void _renderChJammer();
  void _renderHopper();
  static const uint8_t* _jamChannels(int mode, int& count);

  // MouseJack
  void _setupMjScan();
  void _stepMjScan();
  void _fingerprintMj(const uint8_t* buf, uint8_t len, uint8_t ch);
  void _addMjTarget(const uint8_t* addr, uint8_t addrLen, uint8_t ch, uint8_t type);
  void _injectMjText(int targetIdx, const String& text);
  void _renderMjScan();

  void _msTransmit(const MjTarget& t, uint8_t mod, uint8_t key);
  void _logTransmit(const MjTarget& t, uint8_t mod, uint8_t key);
};
