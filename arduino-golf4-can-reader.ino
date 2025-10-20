#include <SPI.h>
#include "can_handler.h"
#include <Encoder.h>
#include <config.h>

Encoder encoder(ENCODER_PIN1, ENCODER_PIN2);
int8_t position = 0;
int8_t oldEnc = 0;
const uint8_t maxEnc = 6;

String FIS_WRITE_line1 = "";
String FIS_WRITE_line2 = "";
long FIS_WRITE_rotary_position_line1 = -8;
long FIS_WRITE_rotary_position_line2 = -8;
int FIS_WRITE_line = 1;
long FIS_WRITE_last_refresh = 0;
int FIS_WRITE_nl = 0;
int FIS_WRITE_ENA_STATUS = 0;
uint8_t FIS_WRITE_CRC = 0;

uint16_t smallStringCount = 0;
uint16_t refreshClusterTime = 300;

uint16_t rpm = 0;
int8_t coolantTemp = 0;
float absSpeed_kmh = 0;
float lastSpeed = 0;
float accelTime = 0;
float holdUntil = 0;
bool reachedSpeed = false;
float speedRange = 100.0;
static uint32_t startTime = 0;

const int points = 53;
float rpmTable[points] = {
  800,900,1000,1100,1200,1300,1400,1500,1600,1700,
  1800,1900,2000,2100,2200,2300,2400,2500,2600,2700,
  2800,2900,3000,3100,3200,3300,3400,3500,3600,3700,
  3800,3900,4000,4100,4200,4300,4400,4500,4600,4700,
  4800,4900,5000,5100,5200,5300,5400,5500,5600,5700,
  5800,5900,6000
};
float torqueTable[points] = {
  95,105,115,125,135,145,155,162,167,170,
  172,173,173.5,174,174.5,174.5,174,174,174,174,
  174,174,174,174,174,174,174,174,174,173.5,
  173,172,170,168,165,160,155,148,140,132,
  125,118,110,102,95,88,82,78,75,72,
  70,68,65
};
const float HP_CONV = 1.34102209;    // kW -> metric hp
float torqueNm = 0;
float lastTorqueNm = 0;
float powerKW = 0;
float powerHP = 0;

//WRITE TO CLUSTER
void FIS_WRITE_sendTEXT(String FIS_WRITE_line1, String FIS_WRITE_line2);
void FIS_WRITE_sendByte(int Bit);
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

  long newEnc = encoder.read() / 4;  // 4 pulses per click
  if (newEnc != oldEnc) {
    int step = (newEnc > oldEnc) ? 1 : -1;
    position += step;

    if (position > maxEnc){
      position = 1;
    }
    if (position < 1){
      position = maxEnc;
    }

    speedRange = position == 3 ? 100.0f : 60.0f;
    oldEnc = newEnc;
  }

  if (CAN_HasMessage()) {
    unsigned long id;
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
          if (absSpeed_kmh < 1.0 && rpm > 4000) {
            startTime = millis();
          }

          if (absSpeed_kmh >= speedRange && startTime > 0) {
            accelTime = (millis() - startTime) / 1000.0;
            reachedSpeed = true;
            holdUntil = millis() + 5000;
          }
        }

        if (absSpeed_kmh < 1.0 && reachedSpeed) {
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
      if (reachedSpeed) {
        if (millis() > holdUntil) {
          reachedSpeed = false;
          accelTime = 0;
          startTime = 0;
        }
      }
      else {
        if (absSpeed_kmh < 1.0f || millis() - startTime > 30000) {
          accelTime = 0;
          startTime = 0;
        }
      }

      FIS_WRITE_line1 = centerString8("0-100KMH");
      FIS_WRITE_line2 = centerString8(String(accelTime) + "S");

      break;
    case 4:
      if (reachedSpeed) {
        if (millis() > holdUntil) {
          reachedSpeed = false;
          accelTime = 0;
          startTime = 0;
        }
      }
      else {
        if (absSpeed_kmh < 1.0f || millis() - startTime > 30000) {
          accelTime = 0;
          startTime = 0;
        }
      }

      FIS_WRITE_line1 = centerString8("0-60KMH");
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

  int FIS_WRITE_line1_length = FIS_WRITE_line1.length();
  int FIS_WRITE_line2_length = FIS_WRITE_line2.length();
  String FIS_WRITE_sendline1 = "        ";
  String FIS_WRITE_sendline2 = "        ";

  //refresh cluster each "refreshClusterTime"
  if (millis() - FIS_WRITE_last_refresh > refreshClusterTime && (FIS_WRITE_line1_length > 0 || FIS_WRITE_line2_length > 0)) {
    if (FIS_WRITE_line1_length > 8) {

      for (int i = 0; i < 8; i++) {
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
      for (int i = 0; i < 8; i++) {
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

return;

  int FIS_WRITE_line1_length = FIS_WRITE_line1.length();
  int FIS_WRITE_line2_length = FIS_WRITE_line2.length();
  if (FIS_WRITE_line1_length <= 8) {
    for (int i = 0; i < (8 - FIS_WRITE_line1_length); i++) {
      FIS_WRITE_line1 += " ";
    }
  }
  if (FIS_WRITE_line2_length <= 8) {
    for (int i = 0; i < (8 - FIS_WRITE_line2_length); i++) {
      FIS_WRITE_line2 += " ";
    }
  }

  FIS_WRITE_CRC = (0xFF ^ FIS_WRITE_START);
  FIS_WRITE_startENA();
  FIS_WRITE_sendByte(FIS_WRITE_START);

  for (int i = 0; i <= 7; i++)
  {
    FIS_WRITE_sendByte(0xFF ^ FIS_WRITE_line1[i]);
    FIS_WRITE_CRC += FIS_WRITE_line1[i];
  }

  for (int i = 0; i <= 7; i++)
  {
    FIS_WRITE_sendByte(0xFF ^ FIS_WRITE_line2[i]);
    FIS_WRITE_CRC += FIS_WRITE_line2[i];
  }

  FIS_WRITE_sendByte(FIS_WRITE_CRC % 0x100);

  FIS_WRITE_stopENA();
}

void FIS_WRITE_sendByte(int Byte) {
  static int iResult[8];
  for (int i = 0; i <= 7; i++)
  {
    iResult[i] = Byte % 2;
    Byte = Byte / 2;
  }

  for (int i = 7; i >= 0; i--) {
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

String centerString8(const String& input) {
    String result = "        "; // 8 whitespace
    int len = input.length();
    if (len >= 8) {
        result = input.substring(0, 8);
    }
    else {
      int totalPadding = 8 - len;
      int paddingLeft, paddingRight;

      if (len % 2 == 0) {
          paddingLeft = totalPadding / 2;
          paddingRight = totalPadding - paddingLeft;
      } else {
          paddingLeft = totalPadding / 2 + 1;
          paddingRight = totalPadding - paddingLeft;
      }

      int index = 0;
      for (int i = 0; i < paddingLeft; i++){
        result[index++] = ' ';
      }

      for (int i = 0; i < len; i++){
        result[index++] = input[i];
      }

      for (int i = 0; i < paddingRight; i++){
        result[index++] = ' ';
      }
    }

    return result;
}

uint8_t getCoolantTemp(uint8_t AA) {
    if (AA <= 0x3D) {
      return map(AA, 0x01, 0x3D, -45, 0);       // -45…0°C
    }
    if (AA <= 0x72) {
      return map(AA, 0x3D, 0x72, 0, 40);        // 0…40°C
    }
    if (AA <= 0xED) {
      return map(AA, 0x72, 0xED, 40, 130);      // 40…130°C
    }

    return 130;                                 // out of range
}

float getTorque(float rpm) {
  if (rpm <= rpmTable[0]){
    return torqueTable[0];
  }

  if (rpm >= rpmTable[points-1]){
    return torqueTable[points-1];
  }

  for (int i = 0; i < points - 1; i++) {
    if (rpm >= rpmTable[i] && rpm <= rpmTable[i + 1]) {
      float t = (rpm - rpmTable[i]) / (rpmTable[i + 1] - rpmTable[i]);

      return torqueTable[i] + t * (torqueTable[i + 1] - torqueTable[i]);
    }
  }

  return torqueTable[points-1];
}