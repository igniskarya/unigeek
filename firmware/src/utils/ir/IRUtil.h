//
// IR Utility — send/receive infrared signals
// Reference: Bruce firmware (https://github.com/pr3y/Bruce)
//

#pragma once
#include <Arduino.h>
#include <IRsend.h>
#include <IRrecv.h>

class IRUtil
{
public:
  static constexpr uint8_t MAX_SIGNALS = 30;
  static constexpr uint16_t IR_FREQUENCY = 38000;

  struct Signal {
    String name;
    bool isRaw;
    uint16_t frequency;
    // raw data stored as space-separated string of durations
    String rawData;
    // parsed protocol data
    String protocol;
    uint32_t address;
    uint32_t command;
    uint16_t bits;
  };

  void beginTx(int8_t txPin);
  void beginRx(int8_t rxPin);
  void end();

  // Receive — returns true if a signal was captured
  bool receive(Signal& out);
  void resumeReceive();

  // Send
  void sendSignal(const Signal& sig);
  void sendRaw(const uint16_t* data, uint16_t length, uint16_t freq);

  // TV-B-Gone
  void startTvBGone(uint8_t region, void (*progressCb)(uint8_t current, uint8_t total) = nullptr,
                     bool (*cancelCb)() = nullptr);

  // File I/O
  static uint8_t loadFile(const String& content, Signal* signals, uint8_t maxCount);
  static String saveToString(const Signal* signals, uint8_t count);

  // Display helpers
  static String signalLabel(const Signal& sig);

private:
  IRsend* _sender = nullptr;
  IRrecv* _receiver = nullptr;
  int8_t _txPin = -1;
  int8_t _rxPin = -1;
};
