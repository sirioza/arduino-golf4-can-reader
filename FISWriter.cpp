#include "FISWriter.h"

#define FIS_WRITE_PULSEW 50
#define FIS_WRITE_STARTPULSEW 100
#define FIS_WRITE_START 15 //something like address, first byte is always 15

FISWriter::FISWriter(uint8_t enaPin, uint8_t clkPin, uint8_t dataPin)
  : _enaPin(enaPin), _clkPin(clkPin), _dataPin(dataPin),
    _rotaryPosLine1(-8), _rotaryPosLine2(-8),
    _lastRefresh(0), _crc(0), _refreshTime(300) { }

void FISWriter::begin() {
  pinMode(_enaPin, OUTPUT);
  pinMode(_clkPin, OUTPUT);
  pinMode(_dataPin, OUTPUT);
  digitalWrite(_enaPin, LOW);
  digitalWrite(_clkPin, HIGH);
  digitalWrite(_dataPin, HIGH);
}

void FISWriter::setRefreshTime(uint16_t refreshTime) {
  _refreshTime = refreshTime;
}

void FISWriter::update(const String& line1, const String& line2) {
  if (millis() - _lastRefresh < _refreshTime) return;
  if (line1.length() == 0 && line2.length() == 0) return;

  String sendLine1 = "        ";
  String sendLine2 = "        ";

  int len1 = line1.length();
  if (len1 > 8) {
    int maxStart1 = len1 - 8;
    if (_rotaryPosLine1 < 0)
    {
      _rotaryPosLine1 = 0;
    }

    for (int i = 0; i < 8; ++i) {
      int idx = _rotaryPosLine1 + i;
      sendLine1[i] = (idx >= 0 && idx < len1) ? line1[idx] : ' ';
    }

    if (_rotaryPosLine1 < maxStart1) {
      _rotaryPosLine1++;
    }
    else {
      _rotaryPosLine1 = -8;
    }
  }
  else {
    _rotaryPosLine1 = -8;
    sendLine1 = centerString8(line1);
  }

  int len2 = line2.length();
  if (len2 > 8) {
    int maxStart2 = len2 - 8;
    if (_rotaryPosLine2 < 0){
     _rotaryPosLine2 = 0;
    }

    for (int i = 0; i < 8; ++i) {
      int idx = _rotaryPosLine2 + i;
      sendLine2[i] = (idx >= 0 && idx < len2) ? line2[idx] : ' ';
    }

    if (_rotaryPosLine2 < maxStart2) {
      _rotaryPosLine2++;
    }
    else {
      _rotaryPosLine2 = -8;
    }
  }
  else {
    _rotaryPosLine2 = -8;
    sendLine2 = centerString8(line2);
  }

  sendText(sendLine1, sendLine2);
  _lastRefresh = millis();
}


void FISWriter::sendText(const String& line1, const String& line2) {  
  Serial.print("|"); Serial.print(line1); Serial.println("|");
  Serial.print("|"); Serial.print(line2); Serial.println("|");

  String l1 = line1, l2 = line2;
  while (l1.length() < 8) l1 += " ";
  while (l2.length() < 8) l2 += " ";

  _crc = (0xFF ^ FIS_WRITE_START);
  startENA();
  sendByte(FIS_WRITE_START);

  for (int i = 0; i < 8; i++) {
    sendByte(0xFF ^ l1[i]);
    _crc += l1[i];
  }

  for (int i = 0; i < 8; i++) {
    sendByte(0xFF ^ l2[i]);
    _crc += l2[i];
  }
  sendByte(_crc % 0x100);
  stopENA();
}

void FISWriter::sendByte(int Byte) {
  for (int i = 7; i >= 0; i--) {
    digitalWrite(_dataPin, (Byte >> i) & 1 ? HIGH : LOW);
    digitalWrite(_clkPin, LOW);
    delayMicroseconds(FIS_WRITE_PULSEW);
    digitalWrite(_clkPin, HIGH);
    delayMicroseconds(FIS_WRITE_PULSEW);
  }
}

void FISWriter::startENA() {
  digitalWrite(_enaPin, HIGH);
}

void FISWriter::stopENA() {
  digitalWrite(_enaPin, LOW);
}

String FISWriter::centerString8(const String& input) {
    String result = "        "; // 8 whitespace
    int8_t len = input.length();
    if (len >= 8) {
        result = input.substring(0, 8);
    }
    else {
      int8_t totalPadding = 8 - len;
      int8_t paddingLeft, paddingRight;

      if (len % 2 == 0) {
          paddingLeft = totalPadding / 2;
          paddingRight = totalPadding - paddingLeft;
      }
      else {
          paddingLeft = totalPadding / 2 + 1;
          paddingRight = totalPadding - paddingLeft;
      }

      int8_t index = 0;
      for (int8_t i = 0; i < paddingLeft; i++){
        result[index++] = ' ';
      }

      for (int8_t i = 0; i < len; i++){
        result[index++] = input[i];
      }

      for (int8_t i = 0; i < paddingRight; i++){
        result[index++] = ' ';
      }
    }

    return result;
}