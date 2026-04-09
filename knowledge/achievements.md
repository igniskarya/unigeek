# Achievements

UniGeek tracks your activity across all features through an achievement system. Every meaningful action — connecting to a network, running an attack, winning a game — can unlock an achievement and award EXP. Your total EXP determines your rank, visible on the Character Screen.

---

## How It Works

Achievements are grouped into **11 domains**, each covering a feature area. Within each domain, achievements are tiered by difficulty:

| Tier | Color | EXP Reward |
|------|-------|-----------|
| Bronze | — | +100 EXP |
| Silver | Light grey | +300 EXP |
| Gold | Yellow | +600 EXP |
| ??? | Hidden | Unlock to discover |

Unlocking an achievement shows a toast notification at the bottom of the screen with the achievement title and EXP earned. Progress is saved to storage at `/unigeek/achievements`.

---

## Ranks

Total possible EXP across all achievements: **45,200**. Ranks are based on percentage of total:

| Rank | EXP Required | % of Total |
|------|-------------|-----------|
| Novice | 0 | — |
| Hacker | 4,500 | ~10% |
| Expert | 15,000 | ~33% |
| Elite | 30,000 | ~66% |
| Legend | 43,000 | ~95% |

Your current rank and a progress bar toward the next rank are shown on the **Character Screen** (accessible from the main menu).

---

## Viewing Achievements

Go to **Utility > Achievements**. The screen has two levels:

1. **Domain list** — shows each domain with how many achievements you've unlocked (e.g. `3/10`) and total EXP earned in that domain
2. **Achievement list** — tap a domain to see all achievements inside it; each row shows the title, tier label (BRONZE / SILVER / GOLD / PLAT), and a description of how to unlock it

Locked top-tier achievements show `???` as the tier and `Unlock to reveal` as the description — their existence is hidden until earned.

---

## Setting Your Agent Title

Any unlocked achievement can be set as your **Agent Title** — a personal badge shown on the Character Screen.

**To set a title:**
1. Go to **Utility > Achievements**
2. Navigate into any domain
3. Scroll to an unlocked achievement
4. **Long-press** (hold ~0.6 seconds) — a "Title saved" confirmation appears

**If the achievement is still locked**, a "Unlock first" message appears instead.

The title is displayed on the Character Screen as:

```
AGENT  MyDevice            [HACKER] Credential Thief
```

If no title has been set, it shows `[RANK] No Title` by default.

---

## Achievement Catalog

Each domain lists a sample of achievements. The full catalog is visible in-app under **Utility > Achievements**.

### WiFi Network (domain 0)

| Title | Tier | How to unlock |
|-------|------|--------------|
| First Contact | Bronze | Scan for nearby WiFi networks |
| Network Hopper | Silver | Connect to 5 different networks |
| Roam Lord | Gold | Connect to 20 different networks |
| ... | | *19 achievements total* |

### WiFi Attacks (domain 1)

| Title | Tier | How to unlock |
|-------|------|--------------|
| Fake Hotspot | Bronze | Start a rogue access point |
| Dark Mirror | Silver | Start an evil twin AP attack |
| Credential Thief | Gold | Capture credentials via evil twin |
| ... | | *32 achievements total* |

### Bluetooth (domain 2)

| Title | Tier | How to unlock |
|-------|------|--------------|
| Bluetooth Scout | Bronze | Scan for nearby BLE devices |
| Blue Storm | Silver | Run BLE spam continuously for 1 min |
| Broken Pairing | Gold | Find a vulnerable WhisperPair device (CVE-2025-36911) |
| ... | | *10 achievements total* |

### Keyboard / HID (domain 3)

| Title | Tier | How to unlock |
|-------|------|--------------|
| Bluetooth Typist | Bronze | Connect as a Bluetooth HID keyboard |
| Script Kiddie | Silver | Run a DuckyScript payload |
| Macro Maestro | Gold | Execute 5 DuckyScript payloads |
| ... | | *6 achievements total* |

### NFC (domain 4)

| Title | Tier | How to unlock |
|-------|------|--------------|
| Card Detected | Bronze | Read an NFC card UID |
| Key Found | Gold | Crack a valid MIFARE sector key |
| Full Dump | Gold | Dump a full NFC card memory |
| ... | | *7 achievements total* |

### IR (domain 5)

| Title | Tier | How to unlock |
|-------|------|--------------|
| Signal Catcher | Bronze | Capture an IR signal with the receiver |
| TV-B-Gone | Silver | Start TV-B-Gone power-off sweep |
| Screen Killer | Gold | Complete a full TV-B-Gone sweep |
| ... | | *5 achievements total* |

### Sub-GHz (domain 6)

| Title | Tier | How to unlock |
|-------|------|--------------|
| RF Listener | Bronze | Receive a Sub-GHz RF signal |
| Frequency Disruptor | Silver | Start RF jamming on a frequency |
| ... | | *5 achievements total* |

### GPS (domain 7)

| Title | Tier | How to unlock |
|-------|------|--------------|
| Locked On | Silver | Get your first GPS position fix |
| Urban Mapper | Gold | Log 100 networks during wardriving |
| ... | | *10 achievements total* |

> WiGLE uploads from both GPS > Wardriving and WiFi > Network > WiGLE count toward the same milestones.

### Utility (domain 8)

| Title | Tier | How to unlock |
|-------|------|--------------|
| Bus Detective | Bronze | Run an I2C bus scan |
| I2C Discovery | Silver | Find a device on the I2C bus |
| ... | | *10 achievements total* |

### Games (domain 9)

| Title | Tier | How to unlock |
|-------|------|--------------|
| First Flight | Bronze | Play Flappy Bird for the first time |
| Air Master | Gold | Score 25 points in Flappy Bird |
| Hex Legend | Gold | Win Hex Decoder on hard difficulty |
| Memory Ace | Gold | Reach round 10 in the Memory game |
| ... | | *21 achievements total* |

### Settings (domain 10)

| Title | Tier | How to unlock |
|-------|------|--------------|
| Identity | Bronze | Change your device name in Settings |
| My Colors | Bronze | Change the UI theme color in Settings |
| ... | | *4 achievements total* |

