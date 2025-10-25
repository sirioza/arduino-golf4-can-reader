#include "Arduino.h"
#include "FISLib.h"

int32_t rotary_position_line1 = -8;
int32_t rotary_position_line2 = -8;
uint8_t line = 1;
uint32_t last_refresh = 0;
uint8_t crc = 0;

FISLib::FISLib(uint8_t ena_pin, uint8_t clk_pin, uint8_t data_pin) {
  _PIN_WRITE_ENA = ena_pin;
  _PIN_WRITE_CLK = clk_pin;
  _PIN_WRITE_DATA = data_pin;

  pinMode(_PIN_WRITE_ENA, OUTPUT);
  digitalWrite(_PIN_WRITE_ENA, LOW);
  pinMode(_PIN_WRITE_CLK, OUTPUT);
  digitalWrite(_PIN_WRITE_CLK, HIGH);
  pinMode(_PIN_WRITE_DATA, OUTPUT);
  digitalWrite(_PIN_WRITE_DATA, HIGH);
}

void FISLib::showText(String line1, String line2, uint16_t &smallStringCount){
  //refresh cluster each "refreshClusterTime"
  uint8_t line1_length = line1.length();
  uint8_t line2_length = line2.length();
  String sendline1 = "        ";
  String sendline2 = "        ";

  //refresh cluster each "refreshClusterTime"
  if (millis() - last_refresh > REFRESH_CLUSTER_TIME && (line1_length > 0 || line2_length > 0)) {
    if (line1_length > 8) {

      for (int8_t i = 0; i < 8; i++) {
        if (rotary_position_line1 + i >= 0 && (rotary_position_line1 + i) < line1_length) {
          sendline1[i] = line1[rotary_position_line1 + i];
        }
      }

      if (rotary_position_line1 < line1_length) {
        rotary_position_line1++;
      }
      else {
        rotary_position_line1 = 0;
      }
    }
    else {
      sendline1 = line1;
    }

    if (line2_length > 8) {
      for (int8_t i = 0; i < 8; i++) {
        if (rotary_position_line2 + i >= 0 && (rotary_position_line2 + i) < line2_length) {
          sendline2[i] = line2[rotary_position_line2 + i];
        }
      }

      if (rotary_position_line2 < line2_length) {
        rotary_position_line2++;
      }
      else {
        rotary_position_line2 = 0;
      }
    }
    else {
      smallStringCount++;
      sendline2 = line2;
    }

    //REFRESH
    sendTEXT(sendline1, sendline2);
    last_refresh = millis();
  }

  //END WRITE TO CLUSTER
}

//WRITE TO CLUSTER
void FISLib::sendTEXT(String line1, String line2) {
  //Serial.print("|"); Serial.print(line1); Serial.println("|");
  //Serial.print("|"); Serial.print(line2); Serial.println("|");

  uint8_t line1_length = line1.length();
  uint8_t line2_length = line2.length();
  if (line1_length <= 8) {
    for (int8_t i = 0; i < (8 - line1_length); i++) {
      line1 += " ";
    }
  }
  if (line2_length <= 8) {
    for (int8_t i = 0; i < (8 - line2_length); i++) {
      line2 += " ";
    }
  }

  crc = (0xFF ^ FIS_WRITE_START);
  startENA();
  sendByte(FIS_WRITE_START);

  for (int8_t i = 0; i <= 7; i++) {
    sendByte(0xFF ^ line1[i]);
    crc += line1[i];
  }

  for (int8_t i = 0; i <= 7; i++) {
    sendByte(0xFF ^ line2[i]);
    crc += line2[i];
  }

  sendByte(crc % 0x100);
  stopENA();
}

void FISLib::sendByte(int8_t byte) {
  static int8_t iResult[8];
  for (int8_t i = 0; i <= 7; i++)
  {
    iResult[i] = byte % 2;
    byte = byte / 2;
  }

  for (int8_t i = 7; i >= 0; i--) {
    switch (iResult[i]) {
      case 1: digitalWrite(_PIN_WRITE_DATA, HIGH);
        break;
      case 0: digitalWrite(_PIN_WRITE_DATA, LOW);
        break;
    }
    digitalWrite(_PIN_WRITE_CLK, LOW);
    delayMicroseconds(FIS_WRITE_PULSEW);
    digitalWrite(_PIN_WRITE_CLK, HIGH);
    delayMicroseconds(FIS_WRITE_PULSEW);
  }
}

//START WRITE TO CLUSTER
void FISLib::startENA() {
  if (!digitalRead(_PIN_WRITE_ENA)) {
    digitalWrite(_PIN_WRITE_ENA, HIGH);
  }
}

//END WRITE TO CLUSTER
void FISLib::stopENA() {
  digitalWrite(_PIN_WRITE_ENA, LOW);
}