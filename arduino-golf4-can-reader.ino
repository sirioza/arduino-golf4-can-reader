#include <SPI.h>
#include "mcp_can.h"
#include <Encoder.h>
#include "config.h"
#include "vehicle_utils.h"
#include "FISWriter.h"

FISWriter FIS(FIS_WRITE_ENA, FIS_WRITE_CLK, FIS_WRITE_DATA, FIS_WRITE_PULSEW);
String line1;
String line2;

MCP_CAN CAN(SPI_CS_PIN);
bool isCanOk = false;

Encoder encoder(ENCODER_CLK, ENCODER_DT);
uint8_t position = 0;
int32_t oldEnc = 0;
const uint8_t MAX_ENC = 8;

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

void setup() {
  Serial.begin(9600);

  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  //pinMode(ENC_BTN_SW, INPUT_PULLUP);

  delay(1200); // time to set Serial before Can

  for (byte i = 0; i < 3; i++) {
    isCanOk = CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK;
    if(isCanOk) {
      CAN.setMode(MCP_NORMAL);
      break;
    }
    else {
      delay(1000);
    }
  }
}

void loop() {

  int32_t newEnc = encoder.read() / 2;
    if (newEnc != oldEnc) {
    int8_t step = (newEnc > oldEnc) ? 1 : -1;
    position += step;

    if (position > MAX_ENC){
      position = 0;
    }
    if (position < 0){
      position = MAX_ENC;
    }

    speedRange_kmh = position == 3 ? 100.0f : 60.0f;
    oldEnc = newEnc;
  }

  //int8_t reading = digitalRead(ENC_BTN_SW); //1, pressed = 0

  if (isCanOk && CAN.checkReceive() == CAN_MSGAVAIL) {
    unsigned long id;
    byte len = 0;
    byte buf[8];
    CAN.readMsgBuf(&id, &len, buf);

    switch (id) {
      case RPM_ID: {
        rpm = (((uint16_t)buf[3] << 8) | buf[4]) / 4;

        if (position == 6 || position == 7){
          torqueNm = getTorque(rpm);
          float omega = 2.0 * 3.14159265358979323846 * rpm / 60.0;
          powerKW = torqueNm * omega / 1000.0;
          powerHP = powerKW * HP_CONV;
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
      line1 = "VW";
      line2 = "GOLF IV 111111";
      break;
    case 1:
      line1 = "RPM";
      line2 = String(rpm);
      break;
    case 2:
      line1 = "COOLANT";
      line2 = String(coolantTemp) + "kC";
      break;
    case 3:
    case 4:
      line1 = "0-" + String(position == 3 ? "100" : "60") + "KMH";
      line2 = String(accelTime) + "S";
      break;
    case 5:
      line1 = "SPEED";
      line2 = String((uint8_t)(absSpeed_kmh + 0.5)) + "KM/H";
      break;
    case 6:
      line1 = "TORQUE";
      line2 = String((uint8_t)(torqueNm + 0.5)) + "NM";
      break;
    case 7:
      line1 = "POWER";
      line2 = String((uint8_t)(powerHP + 0.5)) + "HP";
      break;
    default:
      line1 = "jjjjjjjjUNDEFINED SCREENjjjjjjjj";
      line2 = "        ";
  }

  FIS.update(line1, line2);
}