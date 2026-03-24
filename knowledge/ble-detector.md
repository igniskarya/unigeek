# BLE Detector — How It Works

BLE Detector passively scans for nearby Bluetooth Low Energy devices and identifies potential threats based on known signatures.

## What It Detects

| Type | How It's Identified |
|------|-------------------|
| **Flipper Zero** | Detected by its service UUID; distinguishes real Flipper devices (legit OUI) from spoofed MACs |
| **Credit Card Skimmers** | Matches known Bluetooth module names (HC-05, HC-06, etc.) and suspicious MAC prefixes commonly used in skimming hardware |
| **Apple AirTags / FindMy Trackers** | Identified by Apple manufacturer data signature; shows estimated distance and movement trend (approaching/stationary/moving away) |
| **BitChat App** | Detected by its BLE service UUID (mainnet and testnet); alerts when someone nearby is running the BitChat mesh messaging app |
| **BLE Spam Attacks** | Detects spam patterns targeting Apple, Android, Samsung, and Windows devices by matching against known manufacturer data signatures |

## Usage

1. Go to **Bluetooth > BLE Detector**
2. The scanner runs continuously, listing detected threats in real-time
3. Each entry shows the device type, name (if available), RSSI signal strength, and MAC address
4. Press **BACK** or **Press** to stop scanning
