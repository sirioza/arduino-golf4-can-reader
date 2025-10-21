#include <SPI.h>
#include "can_handler.h"
#include <Encoder.h>
#include <config.h>
#include <vehicle_utils.h>
#include <string_utils.h>

Encoder encoder(ENCODER_PIN1, ENCODER_PIN2);
uint8_t position = 0;
int32_t oldEnc = 0;
const uint8_t MAX_ENC = 6;

String FIS_WRITE_line1 = "";
String FIS_WRITE_line2 = "";
int32_t FIS_WRITE_rotary_position_line1 = -8;
int32_t FIS_WRITE_rotary_position_line2 = -8;
uint8_t FIS_WRITE_line = 1;
uint32_t FIS_WRITE_last_refresh = 0;
uint8_t FIS_WRITE_CRC = 0;

uint16_t smallStringCount = 0;
uint16_t refreshClusterTime = 300;

uint16_t rpm = 0;
int16_t coolantTemp = 0;
float absSpeed_kmh = 0;
float lastSpeed = 0;
float accelTime = 0;
bool reachedSpeed = false;
float speedRange_kmh = 100;
static uint32_t startTime = 0;

const float HP_CONV = 1.34102209;    // kW -> metric hp
float torqueNm = 0;
float lastTorqueNm = 0;
float powerKW = 0;
float powerHP = 0;

//WRITE TO CLUSTER
void FIS_WRITE_sendTEXT(String FIS_WRITE_line1, String FIS_WRITE_line2);
void FIS_WRITE_sendByte(int8_t byte);
void FIS_WRITE_startENA();
void FIS_WRITE_stopENA();
//END WRITE TO CLUSTER

void setup() {

  Serial.begin(9600);

  //WRITE TO CLUSTER
  pinMode(FIS_WRITE_ENA, OUTPUT);
  digitalWrite(FIS_WRITE_ENA, LOW);
  pinMode(FIS_WRITE_CLK, OUTPUT);
  digitalWrite(FIS_WRITE_CLK, HIGH);
  pinMode(FIS_WRITE_DATA, OUTPUT);
  digitalWrite(FIS_WRITE_DATA, HIGH);
  pinMode(ENC_BTN_PIN, INPUT_PULLUP);

  delay(1200); // time to set Serial before Can

  CAN_Init(3);
}

//TODO change coding to +16
void loop() {

  int32_t newEnc = encoder.read() / 4;  // 4 pulses per click
  if (newEnc != oldEnc) {
    int8_t step = (newEnc > oldEnc) ? 1 : -1;
    position += step;

    if (position > MAX_ENC){
      position = 1;
    }
    if (position < 1){
      position = MAX_ENC;
    }

    speedRange_kmh = position == 3 ? 100.0f : 60.0f;
    oldEnc = newEnc;
  }

  if (CAN_HasMessage()) {
    uint32_t id;
    byte len = 0;
    byte buf[8];

    CAN_ReadMessage(id, len, buf);

    switch (id) {
      case RPM_ID: {
        rpm = (((uint16_t)buf[3] << 8) | buf[4]) / 4;

        if (position == 6){
          float torqueNm = getTorque(rpm);
          float omega = 2.0 * 3.14159265358979323846 * rpm / 60.0;
          float powerKW = torqueNm * omega / 1000.0;
          float powerHP = powerKW * HP_CONV;
        }

        break;
      }
      case COOLANT_TEMP_ID: {
        uint8_t AA = buf[1];
        coolantTemp = getCoolantTemp(AA);
        break;
      }
      case ABS_SPEED_ID: {
        absSpeed_kmh = (((uint16_t)buf[3] << 8) | buf[2]) / 200.0f;

        if (position != 3 && position != 4){
          break;
        }

        if (!reachedSpeed) {
          if (absSpeed_kmh < 1.0) {
            startTime = millis();
          }

          if (absSpeed_kmh >= speedRange_kmh && startTime > 0) {
            accelTime = (millis() - startTime) / 1000.0;
            reachedSpeed = true;
          }
        }

        if (absSpeed_kmh < 1.0f && reachedSpeed) {
          reachedSpeed = false;
          accelTime = 0;
          startTime = 0;
        }

        break;
      }
      default:
        break;
    }
  }

  switch (position) {
    case 0:
      FIS_WRITE_line1 = centerString8("WELCOME");
      FIS_WRITE_line2 = centerString8("SIARHEI");

      if (smallStringCount >= 10000/refreshClusterTime){
        smallStringCount = 0;
        position++;
      }
      break;
    case 1:
      FIS_WRITE_line1 = centerString8("RPM");
      FIS_WRITE_line2 = centerString8(String(rpm));
      break;
    case 2:
      FIS_WRITE_line1 = centerString8("COOLANT");
      FIS_WRITE_line2 = centerString8(String(coolantTemp) + "kC");
      break;
    case 3:
    case 4:
      FIS_WRITE_line1 = centerString8("0-" + String(position == 3 ? "100" : "60") + "KMH");
      FIS_WRITE_line2 = centerString8(String(accelTime) + "S");
      break;
    case 5:
      FIS_WRITE_line1 = centerString8("SPEED");
      FIS_WRITE_line2 = centerString8(String((uint8_t)(absSpeed_kmh + 0.5)) + "KM/H");
      break;
    case 6:
      FIS_WRITE_line1 = centerString8(String((uint8_t)(powerHP + 0.5)) + "HP");
      FIS_WRITE_line2 = centerString8(String((uint8_t)(torqueNm + 0.5)) + "NM");
      break;
  }

  uint8_t FIS_WRITE_line1_length = FIS_WRITE_line1.length();
  uint8_t FIS_WRITE_line2_length = FIS_WRITE_line2.length();
  String FIS_WRITE_sendline1 = "        ";
  String FIS_WRITE_sendline2 = "        ";

  //refresh cluster each "refreshClusterTime"
  if (millis() - FIS_WRITE_last_refresh > refreshClusterTime && (FIS_WRITE_line1_length > 0 || FIS_WRITE_line2_length > 0)) {
    if (FIS_WRITE_line1_length > 8) {

      for (int8_t i = 0; i < 8; i++) {
        if (FIS_WRITE_rotary_position_line1 + i >= 0 && (FIS_WRITE_rotary_position_line1 + i) < FIS_WRITE_line1_length) {
          FIS_WRITE_sendline1[i] = FIS_WRITE_line1[FIS_WRITE_rotary_position_line1 + i];
        }
      }

      if (FIS_WRITE_rotary_position_line1 < FIS_WRITE_line1_length) {
        FIS_WRITE_rotary_position_line1++;
      }
      else {
        FIS_WRITE_rotary_position_line1 = 0;
      }
    }
    else {
      FIS_WRITE_sendline1 = FIS_WRITE_line1;
    }

    if (FIS_WRITE_line2_length > 8) {
      for (int8_t i = 0; i < 8; i++) {
        if (FIS_WRITE_rotary_position_line2 + i >= 0 && (FIS_WRITE_rotary_position_line2 + i) < FIS_WRITE_line2_length) {
          FIS_WRITE_sendline2[i] = FIS_WRITE_line2[FIS_WRITE_rotary_position_line2 + i];
        }
      }

      if (FIS_WRITE_rotary_position_line2 < FIS_WRITE_line2_length) {
        FIS_WRITE_rotary_position_line2++;
      }
      else {
        FIS_WRITE_rotary_position_line2 = 0;
      }
    }
    else {
      smallStringCount++;
      FIS_WRITE_sendline2 = FIS_WRITE_line2;
    }

    //REFRESH
    FIS_WRITE_sendTEXT(FIS_WRITE_sendline1, FIS_WRITE_sendline2);
    FIS_WRITE_last_refresh = millis();
  }

  //END WRITE TO CLUSTER
}

//WRITE TO CLUSTER
void FIS_WRITE_sendTEXT(String FIS_WRITE_line1, String FIS_WRITE_line2) {
  Serial.print("|"); Serial.print(FIS_WRITE_line1); Serial.println("|");
  Serial.print("|"); Serial.print(FIS_WRITE_line2); Serial.println("|");

  uint8_t FIS_WRITE_line1_length = FIS_WRITE_line1.length();
  uint8_t FIS_WRITE_line2_length = FIS_WRITE_line2.length();
  if (FIS_WRITE_line1_length <= 8) {
    for (int8_t i = 0; i < (8 - FIS_WRITE_line1_length); i++) {
      FIS_WRITE_line1 += " ";
    }
  }
  if (FIS_WRITE_line2_length <= 8) {
    for (int8_t i = 0; i < (8 - FIS_WRITE_line2_length); i++) {
      FIS_WRITE_line2 += " ";
    }
  }

  FIS_WRITE_CRC = (0xFF ^ FIS_WRITE_START);
  FIS_WRITE_startENA();
  FIS_WRITE_sendByte(FIS_WRITE_START);

  for (int8_t i = 0; i <= 7; i++)
  {
    FIS_WRITE_sendByte(0xFF ^ FIS_WRITE_line1[i]);
    FIS_WRITE_CRC += FIS_WRITE_line1[i];
  }

  for (int8_t i = 0; i <= 7; i++)
  {
    FIS_WRITE_sendByte(0xFF ^ FIS_WRITE_line2[i]);
    FIS_WRITE_CRC += FIS_WRITE_line2[i];
  }

  FIS_WRITE_sendByte(FIS_WRITE_CRC % 0x100);

  FIS_WRITE_stopENA();
}

void FIS_WRITE_sendByte(int8_t byte) {
  static int8_t iResult[8];
  for (int8_t i = 0; i <= 7; i++)
  {
    iResult[i] = byte % 2;
    byte = byte / 2;
  }

  for (int8_t i = 7; i >= 0; i--) {
    switch (iResult[i]) {
      case 1: digitalWrite(FIS_WRITE_DATA, HIGH);
        break;
      case 0: digitalWrite(FIS_WRITE_DATA, LOW);
        break;
    }
    digitalWrite(FIS_WRITE_CLK, LOW);
    delayMicroseconds(FIS_WRITE_PULSEW);
    digitalWrite(FIS_WRITE_CLK, HIGH);
    delayMicroseconds(FIS_WRITE_PULSEW);
  }
}

//START WRITE TO CLUSTER
void FIS_WRITE_startENA() {
  if (!digitalRead(FIS_WRITE_ENA)) {
    digitalWrite(FIS_WRITE_ENA, HIGH);
  }
}

//END WRITE TO CLUSTER
void FIS_WRITE_stopENA() {
  digitalWrite(FIS_WRITE_ENA, LOW);
}