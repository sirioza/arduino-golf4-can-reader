#pragma once
#include <Arduino.h>

class FISWriter {
public:
  FISWriter(uint8_t enaPin, uint8_t clkPin, uint8_t dataPin);

  void update(const String& line1, const String& line2);
  void setRefreshTime(uint16_t refreshTime);
  void begin();

private:
  void sendText(const String& line1, const String& line2);
  void sendByte(int Byte);
  void startENA();
  void stopENA();
  String centerString8(const String& str);

  uint8_t _enaPin;
  uint8_t _clkPin;
  uint8_t _dataPin;
  uint16_t _pulseWidth;
  uint16_t _refreshTime;

  int32_t _rotaryPosLine1;
  int32_t _rotaryPosLine2;
  uint32_t _lastRefresh;
  uint8_t _crc;
};
