#include "NRF24Util.h"

bool NRF24Util::begin(ExtSpiClass* spi, int8_t cePin, int8_t csnPin) {
  if (!spi || cePin < 0 || csnPin < 0) return false;
  end();

  pinMode(csnPin, OUTPUT);
  digitalWrite(csnPin, HIGH);
  pinMode(cePin, OUTPUT);
  digitalWrite(cePin, LOW);
  delay(5);

  _radio = new RF24((uint8_t)cePin, (uint8_t)csnPin);
  if (!_radio->begin(spi)) {
    delete _radio;
    _radio = nullptr;
    return false;
  }

  _started = true;
  return true;
}

void NRF24Util::end() {
  if (_radio) {
    _radio->stopListening();
    _radio->stopConstCarrier();
    _radio->powerDown();
    delete _radio;
    _radio = nullptr;
  }
  _started = false;
}