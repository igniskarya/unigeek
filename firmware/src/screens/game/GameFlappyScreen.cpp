#include "GameFlappyScreen.h"
#include "core/Device.h"
#include "core/ConfigManager.h"
#include "core/ScreenManager.h"
#include "core/AchievementManager.h"
#include "screens/game/GameMenuScreen.h"
#include "ui/actions/ShowStatusAction.h"

static constexpr float kGravity   = 0.35f;
static constexpr float kFlapForce = -3.2f;
static constexpr uint8_t kGroundH = 6;
static constexpr const char* kHsFile = "/unigeek/games/flappy_hs.txt";

// ── Lifecycle ───────────────────────────────────────────────────────────────

void GameFlappyScreen::onInit()
{
  _loadHighScores();
  render();
}

void GameFlappyScreen::onUpdate()
{
  // Game loop ~30fps
  if (_state == STATE_PLAY) {
    uint32_t now = millis();
    if (now - _lastFrameMs >= 33) {
      _lastFrameMs = now;
      _updateGame();
      render();
    }
#ifdef DEVICE_HAS_KEYBOARD
    while (Uni.Keyboard->available()) {
      char c = Uni.Keyboard->getKey();
      if (c == ' ' || c == '\n') { _flap(); break; }
    }
#endif
  }

  if (!Uni.Nav->wasPressed()) return;
  auto dir = Uni.Nav->readDirection();

  if (_state == STATE_MENU) {
    switch (dir) {
      case INavigation::DIR_UP:   _menuIdx = (_menuIdx - 1 + kMenuItems) % kMenuItems; render(); break;
      case INavigation::DIR_DOWN: _menuIdx = (_menuIdx + 1) % kMenuItems;              render(); break;
      case INavigation::DIR_PRESS:
        if (_menuIdx == 0)      _initGame();
        else if (_menuIdx == 1) { _state = STATE_HIGH_SCORES; render(); }
        else                    Screen.setScreen(new GameMenuScreen());
        break;
      case INavigation::DIR_BACK: Screen.setScreen(new GameMenuScreen()); break;
      default: break;
    }
  } else if (_state == STATE_HIGH_SCORES) {
    if (dir != INavigation::DIR_NONE) { _state = STATE_MENU; render(); }
  } else if (_state == STATE_PLAY) {
    if (dir == INavigation::DIR_PRESS || dir == INavigation::DIR_UP) _flap();
    else if (dir == INavigation::DIR_BACK) { _state = STATE_MENU; render(); }
  } else if (_state == STATE_RESULT) {
    if (dir != INavigation::DIR_NONE) {
      _state = STATE_MENU; _menuIdx = 0; render();
    }
  }
}

void GameFlappyScreen::onRender()
{
  if      (_state == STATE_MENU)         _renderMenu();
  else if (_state == STATE_PLAY)         _renderPlay();
  else if (_state == STATE_HIGH_SCORES)  _renderHighScores();
  else                                   _renderResult();
}

// ── Game Logic ──────────────────────────────────────────────────────────────

void GameFlappyScreen::_initGame()
{
  uint16_t h = bodyH();

  // Scale gap and speed to screen size
  _gapH         = h / 3;
  _pipeSpeed    = max(1, (int)(bodyW() / 40));
  _pipeInterval = max(800, 1800 - bodyW() * 4);

  _birdY     = h / 2.0f;
  _velocity  = 0;
  _score     = 0;
  _pipeCount = 0;
  _pipeTimer = 0;
  _lastFrameMs = millis();
  _state     = STATE_PLAY;

  int n = Achievement.inc("flappy_first_play");
  if (n == 1) Achievement.unlock("flappy_first_play");

  // Spawn first pipe ahead
  _spawnPipe();
  render();
}

void GameFlappyScreen::_flap()
{
  _velocity = kFlapForce;
}

void GameFlappyScreen::_spawnPipe()
{
  if (_pipeCount >= kMaxPipes) return;

  uint16_t h     = bodyH() - kGroundH;
  uint8_t  minGY = _gapH / 2 + 4;
  uint8_t  maxGY = h - _gapH / 2 - 4;
  uint8_t  gapY  = minGY + random(maxGY - minGY + 1);

  _pipes[_pipeCount++] = { (int16_t)bodyW(), gapY, false };
}

void GameFlappyScreen::_updateGame()
{
  // Bird physics
  _velocity += kGravity;
  _birdY    += _velocity;

  // Pipe movement
  for (uint8_t i = 0; i < _pipeCount; i++) {
    _pipes[i].x -= _pipeSpeed;

    // Score when bird passes pipe center
    int birdX = 16;
    if (!_pipes[i].scored && _pipes[i].x + kPipeW < birdX) {
      _pipes[i].scored = true;
      _score++;
      if (Uni.Speaker && !Uni.Speaker->isPlaying()) Uni.Speaker->playRandomTone(0, 50);
      if (_score == 10)  Achievement.unlock("flappy_score_10");
      if (_score == 25)  Achievement.unlock("flappy_score_25");
      if (_score == 50)  Achievement.unlock("flappy_score_50");
      if (_score == 100) Achievement.unlock("flappy_score_100");
    }
  }

  // Remove off-screen pipes
  while (_pipeCount > 0 && _pipes[0].x < -kPipeW) {
    for (uint8_t i = 1; i < _pipeCount; i++)
      _pipes[i - 1] = _pipes[i];
    _pipeCount--;
  }

  // Spawn new pipes
  _pipeTimer += 33;
  if (_pipeTimer >= _pipeInterval) {
    _pipeTimer = 0;
    _spawnPipe();
  }

  // Collision
  if (_checkCollision()) {
    _lastScore = _score;
    if (_score > _bestScore) _bestScore = _score;
    _isNewHigh = false;
    _saveHighScore();
    if (Uni.Speaker) Uni.Speaker->playLose();
    ShowStatusAction::show("Game Over!", 1000);
    _state     = STATE_RESULT;
    render();
  }
}

bool GameFlappyScreen::_checkCollision()
{
  uint16_t h = bodyH() - kGroundH;
  int birdX = 16;
  int by    = (int)_birdY;

  // Floor / ceiling
  if (by <= 0 || by + kBirdH >= h) return true;

  // Pipes
  for (uint8_t i = 0; i < _pipeCount; i++) {
    int px = _pipes[i].x;
    // Check horizontal overlap
    if (birdX + kBirdW <= px || birdX >= px + kPipeW) continue;
    // Check if inside gap
    int gapTop = _pipes[i].gapY - _gapH / 2;
    int gapBot = _pipes[i].gapY + _gapH / 2;
    if (by < gapTop || by + kBirdH > gapBot) return true;
  }

  return false;
}

// ── Render ──────────────────────────────────────────────────────────────────

void GameFlappyScreen::_renderMenu()
{
  auto& lcd = Uni.Lcd;
  bool firstTime = (_prevState != STATE_MENU);
  if (firstTime) {
    lcd.fillRect(bodyX(), bodyY(), bodyW(), bodyH(), TFT_BLACK);
    _prevState   = STATE_MENU;
    _lastMenuIdx = -1;
  }

  static constexpr const char* items[kMenuItems] = {"Play", "High Scores", "Exit"};
  lcd.setTextSize(2);
  const int lineH  = lcd.fontHeight() + 6;
  const int startY = (bodyH() - kMenuItems * lineH) / 2 - 8;

  for (int i = 0; i < kMenuItems; i++) {
    bool selNow  = (i == _menuIdx);
    bool selPrev = (i == _lastMenuIdx);
    if (!firstTime && selNow == selPrev) continue;

    Sprite sp(&lcd);
    sp.createSprite(bodyW(), lineH);
    sp.fillSprite(TFT_BLACK);
    sp.setTextDatum(MC_DATUM);
    sp.setTextSize(2);
    sp.setTextColor(selNow ? Config.getThemeColor() : TFT_WHITE, TFT_BLACK);
    sp.drawString(items[i], bodyW() / 2, lineH / 2);
    sp.pushSprite(bodyX(), bodyY() + startY + i * lineH);
    sp.deleteSprite();
  }

  _lastMenuIdx = _menuIdx;

  if (firstTime) {
    lcd.setTextSize(1);
    lcd.setTextDatum(BC_DATUM);
    char buf[24];
    if (_lastScore > 0 && _bestScore > 0) {
      lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
      snprintf(buf, sizeof(buf), "Last: %d", _lastScore);
      lcd.drawString(buf, bodyX() + bodyW() / 2, bodyY() + bodyH() - 12);
      lcd.setTextColor(_lastScore == _bestScore ? TFT_YELLOW : TFT_DARKGREY, TFT_BLACK);
      snprintf(buf, sizeof(buf), "Best: %d", _bestScore);
      lcd.drawString(buf, bodyX() + bodyW() / 2, bodyY() + bodyH() - 2);
    } else if (_bestScore > 0) {
      lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
      snprintf(buf, sizeof(buf), "Best: %d", _bestScore);
      lcd.drawString(buf, bodyX() + bodyW() / 2, bodyY() + bodyH() - 2);
    }
  }
}

void GameFlappyScreen::_renderPlay()
{
  if (_prevState != STATE_PLAY) {
    Uni.Lcd.fillRect(bodyX(), bodyY(), bodyW(), bodyH(), TFT_BLACK);
    _prevState = STATE_PLAY;
  }

  uint16_t w = bodyW();
  uint16_t h = bodyH();
  int birdX  = 16;
  int by     = (int)_birdY;

  const uint8_t kBands    = 4;
  const int     bandH     = (h + kBands - 1) / kBands;

  for (uint8_t b = 0; b < kBands; b++) {
    int yStart = b * bandH;
    int yEnd   = yStart + bandH;
    if (yEnd > h) yEnd = h;
    int thisBandH = yEnd - yStart;
    if (thisBandH <= 0) continue;

    Sprite sp(&Uni.Lcd);
    sp.createSprite(w, thisBandH);
    sp.fillSprite(TFT_BLACK);

    int groundTop = h - kGroundH;
    if (yEnd > groundTop) {
      int gy = groundTop - yStart;
      if (gy < 0) gy = 0;
      sp.fillRect(0, gy, w, thisBandH - gy, TFT_BROWN);
      for (int gx = 0; gx < w; gx += 8)
        sp.fillRect(gx, gy, 4, 2, TFT_OLIVE);
    }

    for (uint8_t i = 0; i < _pipeCount; i++) {
      int px     = _pipes[i].x;
      int gapTop = _pipes[i].gapY - _gapH / 2;
      int gapBot = _pipes[i].gapY + _gapH / 2;

      if (gapTop > 0)
        sp.fillRect(px, 0 - yStart, kPipeW, gapTop, TFT_DARKGREEN);
      sp.fillRect(px - 1, gapTop - 3 - yStart, kPipeW + 2, 3, TFT_GREEN);

      int botH = h - kGroundH - gapBot;
      if (botH > 0)
        sp.fillRect(px, gapBot - yStart, kPipeW, botH, TFT_DARKGREEN);
      sp.fillRect(px - 1, gapBot - yStart, kPipeW + 2, 3, TFT_GREEN);
    }

    sp.fillRect(birdX, by - yStart, kBirdW, kBirdH, TFT_YELLOW);
    sp.fillRect(birdX + kBirdW - 3, by + 1 - yStart, 2, 2, TFT_BLACK);
    sp.fillRect(birdX + kBirdW, by + 3 - yStart, 3, 2, TFT_ORANGE);

    if (b == 0) {
      char buf[8];
      snprintf(buf, sizeof(buf), "%d", _score);
      sp.setTextDatum(TC_DATUM);
      sp.setTextSize(2);
      sp.setTextColor(TFT_WHITE);
      sp.drawString(buf, w / 2, 2);
    }

    sp.pushSprite(bodyX(), bodyY() + yStart);
    sp.deleteSprite();
  }
}

void GameFlappyScreen::_renderResult()
{
  if (_prevState == STATE_RESULT) return;
  _prevState = STATE_RESULT;

  auto& lcd = Uni.Lcd;
  lcd.fillRect(bodyX(), bodyY(), bodyW(), bodyH(), TFT_BLACK);
  lcd.setTextDatum(MC_DATUM);
  lcd.setTextSize(2);

  lcd.setTextColor(TFT_RED, TFT_BLACK);
  lcd.drawString("Game Over!", bodyX() + bodyW() / 2, bodyY() + bodyH() / 2 - 24);

  char buf[24];
  lcd.setTextSize(1);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  snprintf(buf, sizeof(buf), "Score: %d", _score);
  lcd.drawString(buf, bodyX() + bodyW() / 2, bodyY() + bodyH() / 2);

  if (_isNewHigh) {
    lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
    lcd.drawString("NEW HIGH!", bodyX() + bodyW() / 2, bodyY() + bodyH() / 2 + 14);
  } else {
    lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
    snprintf(buf, sizeof(buf), "Best: %d", _bestScore);
    lcd.drawString(buf, bodyX() + bodyW() / 2, bodyY() + bodyH() / 2 + 14);
  }

  lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
  lcd.setTextDatum(BC_DATUM);
  lcd.drawString("Press to continue", bodyX() + bodyW() / 2, bodyY() + bodyH());
}

// ── High Score Storage ───────────────────────────────────────────────────────

void GameFlappyScreen::_loadHighScores()
{
  memset(_topScores, 0, sizeof(_topScores));
  if (!Uni.Storage || !Uni.Storage->exists(kHsFile)) return;
  String data = Uni.Storage->readFile(kHsFile);
  int pos = 0;
  for (uint8_t i = 0; i < kTopN; i++) {
    int nl = data.indexOf('\n', pos);
    if (nl < 0) break;
    _topScores[i] = data.substring(pos, nl).toInt();
    pos = nl + 1;
  }
  _bestScore = _topScores[0];
}

void GameFlappyScreen::_saveHighScore()
{
  // Check if score belongs in top N
  if (_score <= 0) return;
  bool entered = false;
  for (uint8_t i = 0; i < kTopN; i++) {
    if (_score > _topScores[i]) {
      // Shift down
      for (uint8_t j = kTopN - 1; j > i; j--)
        _topScores[j] = _topScores[j - 1];
      _topScores[i] = _score;
      _isNewHigh    = true;
      entered       = true;
      break;
    }
  }
  if (!entered) return;

  _bestScore = _topScores[0];

  if (!Uni.Storage) return;
  String data;
  for (uint8_t i = 0; i < kTopN; i++) {
    data += String(_topScores[i]);
    data += '\n';
  }
  Uni.Storage->makeDir("/unigeek/games");
  Uni.Storage->writeFile(kHsFile, data.c_str());
}

void GameFlappyScreen::_renderHighScores()
{
  if (_prevState == STATE_HIGH_SCORES) return;
  _prevState = STATE_HIGH_SCORES;

  auto& lcd = Uni.Lcd;
  lcd.fillRect(bodyX(), bodyY(), bodyW(), bodyH(), TFT_BLACK);

  lcd.setTextSize(1);
  lcd.setTextDatum(TC_DATUM);
  lcd.setTextColor(Config.getThemeColor(), TFT_BLACK);
  lcd.drawString("Top Scores", bodyX() + bodyW() / 2, bodyY() + 2);

  const int lineH  = 14;
  const int startY = 18;
  char buf[16];
  for (uint8_t i = 0; i < kTopN; i++) {
    uint16_t col = (i == 0) ? TFT_YELLOW : TFT_WHITE;
    if (_topScores[i] == 0) col = TFT_DARKGREY;
    lcd.setTextColor(col, TFT_BLACK);
    lcd.setTextDatum(TL_DATUM);
    snprintf(buf, sizeof(buf), "#%u", i + 1);
    lcd.drawString(buf, bodyX() + 8, bodyY() + startY + i * lineH);
    lcd.setTextDatum(TR_DATUM);
    if (_topScores[i] > 0)
      snprintf(buf, sizeof(buf), "%d", _topScores[i]);
    else
      snprintf(buf, sizeof(buf), "--");
    lcd.drawString(buf, bodyX() + bodyW() - 8, bodyY() + startY + i * lineH);
  }

  lcd.setTextSize(1);
  lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
  lcd.setTextDatum(BC_DATUM);
  lcd.drawString("Press to go back", bodyX() + bodyW() / 2, bodyY() + bodyH() - 1);
}