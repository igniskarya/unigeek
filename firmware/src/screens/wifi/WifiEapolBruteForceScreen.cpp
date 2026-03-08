#include "WifiEapolBruteForceScreen.h"
#include "core/Device.h"
#include "core/ScreenManager.h"
#include "screens/wifi/WifiMenuScreen.h"
#include "ui/actions/ShowStatusAction.h"

#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstring>

// ── Built-in test wordlist (50 common passwords, all ≥ 8 chars) ───────────

static const char* const kTestPasswords[] = {
  "12345678",  "123456789", "1234567890", "11111111",  "00000000",
  "password",  "password1", "passw0rd",   "pass1234",  "iloveyou",
  "sunshine",  "princess",  "football",   "baseball",  "superman",
  "welcome1",  "letmein1",  "trustno1",   "admin123",  "hello123",
  "qwerty123", "qwertyui",  "asdfghjk",   "zxcvbnm1",  "starwars",
  "monkey123", "dragon123", "chocolate",  "internet",  "wireless",
  "wifi1234",  "router12",  "netgear1",   "linksys1",  "network1",
  "connect1",  "computer",  "michael1",   "jessica1",  "abc12345",
  "test1234",  "87654321",  "11223344",   "12344321",  "99999999",
  "88888888",  "55555555",  "12121212",   "homewifi",  "mywifi123",
  "1234asdf",  "asdf1234",  "1234qwer",   "qwer1234",  "qwerasdf"
};
static constexpr int kTestPasswordCount = 55;

// ── Statics ───────────────────────────────────────────────────────────────

const char* WifiEapolBruteForceScreen::PCAP_DIR = "/unigeek/wifi/eapol";
const char* WifiEapolBruteForceScreen::PASS_DIR = "/unigeek/utility/passwords";

WifiEapolBruteForceScreen::CrackCtx     WifiEapolBruteForceScreen::_ctx       = {};
TaskHandle_t                             WifiEapolBruteForceScreen::_taskHandle = nullptr;

// ── File-scope PCAP constants ─────────────────────────────────────────────

static const uint8_t kSnapSig[8]  = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8E};
static constexpr uint16_t kKiAck     = 0x0080;
static constexpr uint16_t kKiMic     = 0x0100;
static constexpr uint16_t kKiInstall = 0x0040;
static constexpr uint16_t kKiSecure  = 0x0200;

// ── File-scope helpers ────────────────────────────────────────────────────

static bool pcapRead32(File& f, uint32_t& v) {
  uint8_t b[4];
  if (f.read(b, 4) != 4) return false;
  v = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
  return true;
}

static int findSnap(const uint8_t* frm, uint16_t len) {
  for (uint16_t i = 0; i + 8 <= len; i++) {
    bool ok = true;
    for (int k = 0; k < 8; k++) if (frm[i + k] != kSnapSig[k]) { ok = false; break; }
    if (ok) return (int)i;
  }
  return -1;
}

// PRF-512 (WPA2 PTK derivation): 4 × HMAC-SHA1 rounds, reuses caller's context
static void prf512(mbedtls_md_context_t& ctx, const uint8_t* pmk,
                   const uint8_t* prf_data, uint8_t* ptk) {
  static const char label[] = "Pairwise key expansion"; // sizeof() includes '\0' separator
  uint8_t digest[20];
  uint8_t ctr = 0;
  size_t  pos = 0;
  while (pos < 64) {
    mbedtls_md_hmac_starts(&ctx, pmk, 32);
    mbedtls_md_hmac_update(&ctx, (const uint8_t*)label, sizeof(label));
    mbedtls_md_hmac_update(&ctx, prf_data, 76);
    mbedtls_md_hmac_update(&ctx, &ctr, 1);
    mbedtls_md_hmac_finish(&ctx, digest);
    size_t c = (64 - pos < 20) ? 64 - pos : 20;
    memcpy(ptk + pos, digest, c);
    pos += c;
    ctr++;
  }
}

// ── Destructor ────────────────────────────────────────────────────────────

WifiEapolBruteForceScreen::~WifiEapolBruteForceScreen() {
  _stopCrack();
}

// ── Lifecycle ─────────────────────────────────────────────────────────────

void WifiEapolBruteForceScreen::_showMenu() {
  _state = STATE_MENU;

  const char* pcapBase = strrchr(_selectedPcap, '/');
  _pcapSub     = (_selectedPcap[0]     ? String(pcapBase ? pcapBase + 1 : _selectedPcap)     : "(not selected)");
  const char* wlBase  = strrchr(_selectedWordlist, '/');
  _wordlistSub = (_selectedWordlist[0] ? String(wlBase   ? wlBase   + 1 : _selectedWordlist) : "(not selected)");

  _menuItems[0] = {"PCAP File", _pcapSub.c_str()};
  _menuItems[1] = {"Wordlist",  _wordlistSub.c_str()};
  _menuItems[2] = {"Start",     nullptr};
  setItems(_menuItems, 3);
}

void WifiEapolBruteForceScreen::onInit() {
  if (!Uni.Storage || !Uni.Storage->isAvailable()) {
    ShowStatusAction::show("No storage available.");
    Screen.setScreen(new WifiMenuScreen());
    return;
  }

  Uni.Storage->makeDir(PASS_DIR);
  _selectedPcap[0]      = '\0';
  _selectedWordlist[0]  = '\0';
  _showMenu();
}

void WifiEapolBruteForceScreen::onUpdate() {
  if (_state == STATE_CRACKING) {
    if (Uni.Nav->wasPressed()) {
      auto dir = Uni.Nav->readDirection();
      if (dir == INavigation::DIR_BACK || dir == INavigation::DIR_PRESS) {
        _stopCrack();
        return;
      }
    }
    if (_ctx.done) {
      _state = STATE_DONE;
      _taskHandle = nullptr;
      if (_ctx.found) { if (Uni.Speaker) Uni.Speaker->playWin(); }
      else            { if (Uni.Speaker) Uni.Speaker->playLose(); }
      render();
    } else {
      render();
    }
    return;
  }

  if (_state == STATE_DONE) {
    if (Uni.Nav->wasPressed()) {
      Uni.Nav->readDirection();
      _showMenu();
    }
    return;
  }

  ListScreen::onUpdate();
}

void WifiEapolBruteForceScreen::onRender() {
  if (_state == STATE_CRACKING) { _renderCracking(); return; }
  if (_state == STATE_DONE)     { _renderDone();     return; }
  ListScreen::onRender();
}

void WifiEapolBruteForceScreen::onBack() {
  if (_state == STATE_SELECT_PCAP || _state == STATE_SELECT_WORDLIST) {
    _showMenu();
    return;
  }
  Screen.setScreen(new WifiMenuScreen());
}

void WifiEapolBruteForceScreen::onItemSelected(uint8_t index) {
  if (_state == STATE_MENU) {
    if (index == 0) {
      // Select PCAP
      if (!_listFiles(PCAP_DIR, ".pcap")) {
        ShowStatusAction::show("No PCAP files found.\nCapture EAPOL first.");
        render();
        return;
      }
      _state = STATE_SELECT_PCAP;
      setItems(_fileItems, (uint8_t)_fileCount);
    } else if (index == 1) {
      // Select wordlist
      _listFiles(PASS_DIR, nullptr);  // always has Test Wordlist even if empty
      _state = STATE_SELECT_WORDLIST;
      setItems(_fileItems, (uint8_t)_fileCount);
    } else if (index == 2) {
      // Start — require both selected
      if (_selectedPcap[0] == '\0') {
        ShowStatusAction::show("Select a PCAP file first.");
        render();
        return;
      }
      if (_selectedWordlist[0] == '\0') {
        ShowStatusAction::show("Select a wordlist first.");
        render();
        return;
      }
      ShowStatusAction::show("Parsing PCAP...", 0);
      if (!_parsePcap(_selectedPcap)) {
        ShowStatusAction::show("No valid handshake\nin PCAP file.");
        render();
        return;
      }
      _startCrack();
    }
    return;
  }

  if (index >= (uint8_t)_fileCount) return;

  if (_state == STATE_SELECT_PCAP) {
    strncpy(_selectedPcap, _filePaths[index], sizeof(_selectedPcap) - 1);
    _showMenu();
    return;
  }

  if (_state == STATE_SELECT_WORDLIST) {
    strncpy(_selectedWordlist, _filePaths[index], sizeof(_selectedWordlist) - 1);
    _showMenu();
    return;
  }

}

// ── File browser ──────────────────────────────────────────────────────────

bool WifiEapolBruteForceScreen::_listFiles(const char* dir, const char* ext) {
  _fileCount = 0;
  if (!Uni.Storage || !Uni.Storage->isAvailable()) return false;

  IStorage::DirEntry entries[MAX_FILES];
  uint8_t count = Uni.Storage->listDir(dir, entries, MAX_FILES);

  for (uint8_t i = 0; i < count && _fileCount < MAX_FILES; i++) {
    if (entries[i].isDir) continue;

    String name  = entries[i].name;
    int    slash  = name.lastIndexOf('/');
    String base   = (slash >= 0) ? name.substring(slash + 1) : name;
    String full   = String(dir) + "/" + base;

    if (ext && !base.endsWith(ext)) continue;

    snprintf(_fileLabels[_fileCount], sizeof(_fileLabels[0]), "%s", base.c_str());
    snprintf(_filePaths[_fileCount],  sizeof(_filePaths[0]),  "%s", full.c_str());
    _fileItems[_fileCount] = {_fileLabels[_fileCount], nullptr};
    _fileCount++;
  }
  // Always append built-in test wordlist when listing passwords (no ext filter)
  if (ext == nullptr && _fileCount < MAX_FILES) {
    snprintf(_fileLabels[_fileCount], sizeof(_fileLabels[0]), "Test Wordlist");
    snprintf(_filePaths[_fileCount],  sizeof(_filePaths[0]),  "__test__");
    _fileItems[_fileCount] = {_fileLabels[_fileCount], "55 entries"};
    _fileCount++;
  }

  return _fileCount > 0;
}

// ── PCAP parser ───────────────────────────────────────────────────────────

bool WifiEapolBruteForceScreen::_parsePcap(const char* path) {
  Handshake& hs = _ctx.hs;
  memset(&hs, 0, sizeof(hs));

  File f = Uni.Storage->open(path, FILE_READ);
  if (!f) return false;

  // Global header (24 bytes)
  uint8_t gh[24];
  if (f.read(gh, 24) != 24) { f.close(); return false; }
  // Verify LE pcap magic D4 C3 B2 A1
  if (!(gh[0] == 0xD4 && gh[1] == 0xC3 && gh[2] == 0xB2 && gh[3] == 0xA1)) {
    f.close(); return false;
  }
  // Link type: bytes 20-23 (LE). 105 = raw 802.11, 127 = radiotap+802.11
  uint32_t linktype = (uint32_t)gh[20] | ((uint32_t)gh[21] << 8) |
                      ((uint32_t)gh[22] << 16) | ((uint32_t)gh[23] << 24);

  uint8_t rec[512];
  bool    gotM1 = false, gotM2 = false;
  uint64_t replayM1 = 0;

  while (f.available() > 16) {
    uint32_t ts, tu, incl, orig;
    if (!pcapRead32(f, ts) || !pcapRead32(f, tu) ||
        !pcapRead32(f, incl) || !pcapRead32(f, orig)) break;

    if (!incl || incl > sizeof(rec)) { f.seek(f.position() + incl); continue; }
    if (f.read(rec, incl) != (int)incl) break;

    // Strip radiotap header for link type 127
    uint16_t off = 0;
    if (linktype == 127) {
      if (incl < 4) continue;
      uint16_t rtLen = (uint16_t)rec[2] | ((uint16_t)rec[3] << 8);
      if (rtLen >= incl) continue;
      off = rtLen;
    }

    const uint8_t* frm  = rec + off;
    uint16_t       flen = (uint16_t)(incl - off);
    if (flen < 24) continue;

    const uint16_t fc     = (uint16_t)frm[0] | ((uint16_t)frm[1] << 8);
    const uint8_t  fcType = (fc & 0x000C) >> 2;
    const uint8_t  fcSub  = (fc & 0x00F0) >> 4;

    // Beacon → extract SSID (only once, first seen)
    if (fcType == 0 && fcSub == 8 && flen >= 36 && hs.ssid[0] == '\0') {
      uint16_t pos = 36;
      while (pos + 2 <= flen) {
        uint8_t id = frm[pos], elen = frm[pos + 1];
        if (pos + 2 + elen > flen) break;
        if (id == 0 && elen > 0 && elen <= 32) {
          memcpy(hs.ssid, frm + pos + 2, elen);
          hs.ssid[elen] = '\0';
          hs.ssid_len   = elen;
          break;
        }
        pos += 2 + elen;
      }
      continue;
    }

    // Only process data frames for EAPOL
    if (fcType != 2) continue;

    int snap = findSnap(frm, flen);
    if (snap < 0) continue;
    if ((uint16_t)(snap + 9) >= flen) continue;

    const uint8_t* eapol = frm + snap + 8;
    if (eapol[1] != 0x03) continue;  // not EAPOL-Key

    uint16_t eap_len = ((uint16_t)eapol[2] << 8) | eapol[3];
    uint16_t total   = 4 + eap_len;
    if ((uint16_t)(snap + 8) + total > flen) continue;

    const uint8_t* key    = eapol + 4;
    uint16_t       ki     = ((uint16_t)key[1] << 8) | key[2];
    bool           ack    = ki & kKiAck;
    bool           mic    = ki & kKiMic;
    bool           inst   = ki & kKiInstall;
    bool           sec    = ki & kKiSecure;
    uint8_t        keyver = ki & 7;

    uint64_t replay = 0;
    for (int i = 0; i < 8; i++) replay = (replay << 8) | key[5 + i];

    // M1: ACK=1, MIC=0 — AP sends ANonce
    if (!gotM1 && ack && !mic) {
      memcpy(hs.ap,     frm + 10, 6);  // addr2 = AP source
      memcpy(hs.sta,    frm + 4,  6);  // addr1 = STA dest
      memcpy(hs.anonce, key + 13, 32);
      replayM1 = replay;
      gotM1    = true;
      continue;
    }

    // M2: ACK=0, MIC=1, INSTALL=0, SECURE=0 — STA sends SNonce + MIC
    if (!gotM2 && !ack && mic && !inst && !sec && keyver >= 1 && keyver <= 3) {
      if (!gotM1) continue;
      if (replay != replayM1) continue;
      // Verify matching MACs: addr2=STA, addr1=AP
      if (memcmp(frm + 10, hs.sta, 6) != 0) continue;
      if (memcmp(frm + 4,  hs.ap,  6) != 0) continue;

      memcpy(hs.snonce, key + 13, 32);
      memcpy(hs.mic,    eapol + 81, 16);
      if (total <= sizeof(hs.eapol)) {
        memcpy(hs.eapol, eapol, total);
        memset(hs.eapol + 81, 0, 16);  // zero MIC field for recomputation
        hs.eapol_len = total;
      }
      gotM2 = true;
    }
  }
  f.close();

  if (!(gotM1 && gotM2)) return false;

  // If SSID missing from beacon, try extracting from filename (BSSID_SSID.pcap)
  if (hs.ssid[0] == '\0') {
    String p   = String(path);
    int    und = p.lastIndexOf('_');
    int    dot = p.lastIndexOf('.');
    if (und >= 0 && dot > und + 1) {
      String part = p.substring(und + 1, dot);
      snprintf(hs.ssid, sizeof(hs.ssid), "%s", part.c_str());
      hs.ssid_len = (uint8_t)strlen(hs.ssid);
    }
    if (hs.ssid[0] == '\0') return false;
  }

  // Build prf_data[76]: min(AP,STA) || max(AP,STA) || min(ANonce,SNonce) || max(ANonce,SNonce)
  uint8_t* p = hs.prf_data;
  if (memcmp(hs.ap, hs.sta, 6) < 0) {
    memcpy(p, hs.ap,  6); p += 6; memcpy(p, hs.sta, 6); p += 6;
  } else {
    memcpy(p, hs.sta, 6); p += 6; memcpy(p, hs.ap,  6); p += 6;
  }
  if (memcmp(hs.anonce, hs.snonce, 32) < 0) {
    memcpy(p, hs.anonce, 32); p += 32; memcpy(p, hs.snonce, 32);
  } else {
    memcpy(p, hs.snonce, 32); p += 32; memcpy(p, hs.anonce, 32);
  }

  hs.valid = true;
  return true;
}

// ── Crack task ────────────────────────────────────────────────────────────

void WifiEapolBruteForceScreen::_startCrack() {
  memset(_ctx.foundPass, 0, sizeof(_ctx.foundPass));
  memset(_ctx.curPass,   0, sizeof(_ctx.curPass));
  _ctx.stop      = false;
  _ctx.done      = false;
  _ctx.found     = false;
  _ctx.tested    = 0;
  _ctx.bytesDone = 0;
  _ctx.speed     = 0.0f;
  strncpy(_ctx.wordlistPath, _selectedWordlist, sizeof(_ctx.wordlistPath) - 1);

  // Get file size for progress bar (test wordlist uses entry count instead)
  if (strcmp(_selectedWordlist, "__test__") == 0) {
    _ctx.fileSize = kTestPasswordCount;
  } else {
    fs::File f = Uni.Storage->open(_selectedWordlist, FILE_READ);
    _ctx.fileSize = f ? f.size() : 0;
    if (f) f.close();
  }

  // Pin to core 0 — Arduino loop() runs on core 1, keeps UI responsive
  xTaskCreatePinnedToCore(_crackTask, "eapol_bf", 8192, &_ctx, 1, &_taskHandle, 0);
  _state = STATE_CRACKING;
  render();
}

void WifiEapolBruteForceScreen::_stopCrack() {
  if (!_taskHandle) return;
  _ctx.stop = true;
  // Wait up to 2s for task to self-exit cleanly
  for (int i = 0; i < 200 && !_ctx.done; i++) {
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  if (!_ctx.done) vTaskDelete(_taskHandle);
  _taskHandle = nullptr;
  if (_state == STATE_CRACKING) {
    _state = STATE_DONE;
    render();
  }
}

void WifiEapolBruteForceScreen::_crackTask(void* param) {
  CrackCtx* ctx = static_cast<CrackCtx*>(param);

  // Init mbedTLS contexts once — reused for all passwords to avoid init overhead
  mbedtls_md_context_t pbkdfCtx, prfCtx;
  const mbedtls_md_info_t* sha1 = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
  mbedtls_md_init(&pbkdfCtx); mbedtls_md_setup(&pbkdfCtx, sha1, 1);
  mbedtls_md_init(&prfCtx);   mbedtls_md_setup(&prfCtx,   sha1, 1);

  char     line[64];
  uint32_t tested = 0;
  uint32_t t0     = millis();

  // ── Helper lambda-like macro to crack one password candidate ──────────────
  // (shared between file and test-wordlist paths)
  #define TRY_PASSWORD(pw, pwlen) do { \
    memcpy(ctx->curPass, (pw), (pwlen) + 1); \
    uint8_t pmk[32], ptk[64], mic[20]; \
    mbedtls_pkcs5_pbkdf2_hmac(&pbkdfCtx, \
      (const uint8_t*)(pw), (pwlen), \
      (const uint8_t*)ctx->hs.ssid, ctx->hs.ssid_len, \
      4096, 32, pmk); \
    prf512(prfCtx, pmk, ctx->hs.prf_data, ptk); \
    mbedtls_md_hmac(sha1, ptk, 16, ctx->hs.eapol, ctx->hs.eapol_len, mic); \
    tested++; ctx->tested = tested; \
    uint32_t _el = millis() - t0; \
    if (_el >= 2000) ctx->speed = tested * 1000.0f / _el; \
    if (memcmp(mic, ctx->hs.mic, 16) == 0) { \
      memcpy(ctx->foundPass, (pw), (pwlen) + 1); \
      ctx->found = true; \
    } \
    vTaskDelay(1); \
  } while (0)

  if (strcmp(ctx->wordlistPath, "__test__") == 0) {
    // Built-in test wordlist — no storage access needed
    for (int i = 0; i < kTestPasswordCount && !ctx->stop && !ctx->found; i++) {
      size_t n = strlen(kTestPasswords[i]);
      strncpy(line, kTestPasswords[i], sizeof(line) - 1);
      line[n] = '\0';
      TRY_PASSWORD(line, n);
      ctx->bytesDone = (uint32_t)(i + 1);  // progress = entries tried / 50
    }
  } else {
    fs::File f = Uni.Storage->open(ctx->wordlistPath, FILE_READ);
    if (f) {
      while (f.available() && !ctx->stop && !ctx->found) {
        size_t n = f.readBytesUntil('\n', line, 63);
        line[n] = '\0';
        while (n > 0 && (line[n - 1] == '\r' || line[n - 1] == '\n')) line[--n] = '\0';
        if (n < 8 || n > 63) { ctx->bytesDone = f.position(); continue; }
        TRY_PASSWORD(line, n);
        ctx->bytesDone = f.position();
      }
      f.close();
    }
  }

  #undef TRY_PASSWORD
  mbedtls_md_free(&pbkdfCtx);
  mbedtls_md_free(&prfCtx);
  ctx->done = true;
  vTaskDelete(nullptr);
}

// ── Render ────────────────────────────────────────────────────────────────

void WifiEapolBruteForceScreen::_renderCracking() {
  TFT_eSprite sp(&Uni.Lcd);
  sp.createSprite(bodyW(), bodyH());
  sp.fillSprite(TFT_BLACK);

  const uint16_t accent = Config.getThemeColor();
  const int      lh     = 14;
  int            y      = 4;

  // SSID row
  sp.setTextDatum(TL_DATUM);
  sp.setTextColor(TFT_DARKGREY, TFT_BLACK);
  sp.drawString("SSID:", 4, y, 1);
  sp.setTextColor(TFT_WHITE, TFT_BLACK);
  sp.drawString(_ctx.hs.ssid, 40, y, 1);
  y += lh;

  // Current password (max 22 chars to avoid overflow)
  char disp[24];
  snprintf(disp, sizeof(disp), "%.23s", _ctx.curPass);
  sp.setTextColor(TFT_DARKGREY, TFT_BLACK);
  sp.drawString("Try:", 4, y, 1);
  sp.setTextColor(accent, TFT_BLACK);
  sp.drawString(disp, 36, y, 1);
  y += lh + 4;

  // Progress bar — 3px padding each side, taller to fit font (8px) + padding
  const int barPad = 3;
  const int barX   = barPad;
  const int barW   = bodyW() - barPad * 2;
  const int barH   = 8 + barPad * 2;   // font height + top/bottom padding
  int pct = (_ctx.fileSize > 0)
    ? (int)((uint64_t)_ctx.bytesDone * 100 / _ctx.fileSize)
    : 0;
  if (pct > 100) pct = 100;

  sp.drawRect(barX, y, barW, barH, TFT_DARKGREY);
  if (pct > 0) sp.fillRect(barX + 1, y + 1, (barW - 2) * pct / 100, barH - 2, accent);

  // Percent label centered on bar — transparent background (no bg color)
  char pctBuf[6];
  snprintf(pctBuf, sizeof(pctBuf), "%d%%", pct);
  sp.setTextDatum(MC_DATUM);
  sp.setTextColor(TFT_WHITE);
  sp.drawString(pctBuf, barX + barW / 2, y + barH / 2, 1);
  y += barH + 6;

  // Speed + count
  char statBuf[48];
  snprintf(statBuf, sizeof(statBuf), "%.1f/s  |  %lu tested",
           (float)_ctx.speed, (unsigned long)_ctx.tested);
  sp.setTextDatum(TC_DATUM);
  sp.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  sp.drawString(statBuf, bodyW() / 2, y, 1);

  // Bottom hint — device-appropriate stop instruction
  sp.setTextDatum(BC_DATUM);
  sp.setTextColor(TFT_DARKGREY, TFT_BLACK);
#ifdef DEVICE_HAS_KEYBOARD
  sp.drawString("BACK / ENTER: Stop", bodyW() / 2, bodyH(), 1);
#else
  sp.drawString("Any btn: Stop", bodyW() / 2, bodyH(), 1);
#endif

  sp.pushSprite(bodyX(), bodyY());
  sp.deleteSprite();
}

void WifiEapolBruteForceScreen::_renderDone() {
  TFT_eSprite sp(&Uni.Lcd);
  sp.createSprite(bodyW(), bodyH());
  sp.fillSprite(TFT_BLACK);

  const int cx = bodyW() / 2;
  const int cy = bodyH() / 2;

  if (_ctx.found) {
    sp.setTextDatum(MC_DATUM);
    sp.setTextColor(TFT_GREEN, TFT_BLACK);
    sp.drawString("PASSWORD FOUND!", cx, cy - 22, 1);
    sp.setTextColor(TFT_WHITE, TFT_BLACK);
    sp.drawString(_ctx.foundPass, cx, cy - 6, 1);
    char buf[32];
    snprintf(buf, sizeof(buf), "%lu tries", (unsigned long)_ctx.tested);
    sp.setTextColor(TFT_DARKGREY, TFT_BLACK);
    sp.drawString(buf, cx, cy + 10, 1);
  } else {
    sp.setTextDatum(MC_DATUM);
    sp.setTextColor(TFT_RED, TFT_BLACK);
    sp.drawString("Not in wordlist", cx, cy - 10, 1);
    char buf[32];
    snprintf(buf, sizeof(buf), "%lu tested", (unsigned long)_ctx.tested);
    sp.setTextColor(TFT_DARKGREY, TFT_BLACK);
    sp.drawString(buf, cx, cy + 6, 1);
  }

  sp.setTextDatum(BC_DATUM);
  sp.setTextColor(TFT_DARKGREY, TFT_BLACK);
  sp.drawString("Any key to continue", cx, bodyH(), 1);

  sp.pushSprite(bodyX(), bodyY());
  sp.deleteSprite();
}
