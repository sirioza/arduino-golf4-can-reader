#include <SPI.h>
#include "mcp_can.h"

//END WRITE TO CLUSTER
//data pin 4
//clk  pin 3
//ena  pin 2
//WRITE TO CLUSTER
#define FIS_WRITE_ENA 2
#define FIS_WRITE_CLK 3
#define FIS_WRITE_DATA 4
#define FIS_WRITE_PULSEW 50
#define FIS_WRITE_STARTPULSEW 100
#define FIS_WRITE_START 15 //something like address, first byte is always 15
//END WRITE TO CLUSTER
//#define BINCODE 000011111011111010101010101110111011011010111100101100111010101010111101101011001011001110110000101010011011111010110100101101101011111010010011
//                000011111011111010101010101110111011011010111100101100111010101010111101101011001011001110110000101010011011111010110100101101101011111010010011

//int BINCODE[144]={
//0,0,0,0,1,1,1,1,
//1,0,1,1,1,1,1,0,
//1,0,1,0,1,0,1,0,
//1,0,1,1,1,0,1,1,
//1,0,1,1,0,1,1,0,
//1,0,1,1,1,1,0,0,
//1,0,1,1,0,0,1,1,
//1,0,1,0,1,0,1,0,
//1,0,1,1,1,1,0,1,
//1,0,1,0,1,1,0,0,
//1,0,1,1,0,0,1,1,
//1,0,1,1,0,0,0,0,
//1,0,1,0,1,0,0,1,
//1,0,1,1,1,1,1,0,
//1,0,1,1,0,1,0,0,
//1,0,1,1,0,1,1,0,
//1,0,1,1,1,1,1,0,
//1,0,0,1,0,0,1,1};

//CLUSTERS ID
#define SPEEDOMETR_ID 0x5A0
#define COOLANT_TEMP_ID 0x288
#define RPM_ID 0x280
#define ABS_SPEED_ID 0x1A0

//WRITE TO CLUSTER
String FIS_WRITE_line1 = "";
String FIS_WRITE_line2 = "";
long FIS_WRITE_rotary_position_line1 = -8;
long FIS_WRITE_rotary_position_line2 = -8;
int FIS_WRITE_line = 1;
long FIS_WRITE_last_refresh = 0;
int FIS_WRITE_nl = 0;
int FIS_WRITE_ENA_STATUS = 0;
uint8_t FIS_WRITE_CRC = 0;
//END WRITE TO CLUSTER

uint8_t screen = 0;
uint16_t smallStringCount = 0;
uint16_t refreshClusterTime = 300;

//WRITE TO CLUSTER
void FIS_WRITE_sendTEXT(String FIS_WRITE_line1, String FIS_WRITE_line2);
void FIS_WRITE_sendByte(int Bit);
void FIS_WRITE_startENA();
void FIS_WRITE_stopENA();
//END WRITE TO CLUSTER

//CAN PARAMS
const int SPI_CS_PIN = 10;   // CS
const int CAN_INT_PIN = 9;   // INT
MCP_CAN CAN(SPI_CS_PIN);

unsigned long delayTime = 10000;
static unsigned long lastUpdate = 0;
static bool paused = false;
static bool isCanOk;

uint16_t  rpm = 0;
uint8_t coolantTemp = 0;

float absSpeed_kmh = 0;
float lastSpeed = 0;
float accelTime = 0;
float holdUntil = 0;
bool reached100 = false;
static uint32_t startTime = 0;

void setup() {
  //WRITE TO CLUSTER
  pinMode(FIS_WRITE_ENA, OUTPUT);
  digitalWrite(FIS_WRITE_ENA, LOW);
  pinMode(FIS_WRITE_CLK, OUTPUT);
  digitalWrite(FIS_WRITE_CLK, HIGH);
  pinMode(FIS_WRITE_DATA, OUTPUT);
  digitalWrite(FIS_WRITE_DATA, HIGH);
  Serial.begin(9600); //10400

  //CAN
  for (byte i = 0; i < 5; i++) {
    isCanOk = CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK;
    if(isCanOk) {
      CAN.setMode(MCP_NORMAL);
      break;
    }
    else {
      delay(1000);
    }
  }

  delay(1200);
}

void loop() {
  if (isCanOk && CAN.checkReceive() == CAN_MSGAVAIL) {
    unsigned long id;
    byte len = 0;
    byte buf[8];

    CAN.readMsgBuf(&id, &len, buf);

    switch (id) {
      case COOLANT_TEMP_ID: {
        uint8_t AA = buf[1];
        coolantTemp = getCoolantTemp(AA);
        break;
      }
      case RPM_ID: {
        rpm = (((uint16_t)buf[3] << 8) | buf[4]) / 4;
        break;
      }
      case ABS_SPEED_ID: {
        //absSpeed_kmh = (((uint16_t)buf[3] << 8) | buf[2]) / 200.0f;
        absSpeed_kmh = buf[3] * 1.33f;

        if (!reached100) {
          if (absSpeed_kmh < 1.0 && rpm > 4000) {
            startTime = millis();
          }

          if (absSpeed_kmh >= 100.0 && startTime > 0) {
            accelTime = (millis() - startTime) / 1000.0;
            reached100 = true;
            holdUntil = millis() + 5000;
          }
        }

        if (absSpeed_kmh < 1.0 && reached100) {
          reached100 = false;
          accelTime = 0;
          startTime = 0;
        }

        break;
      }
    }
  }

  switch (screen) {
    case 0:
      FIS_WRITE_line1 = "WELCOME";
      FIS_WRITE_line2 = "SIARHEI";

      if (smallStringCount >= 10000/refreshClusterTime){
        screen++;
        smallStringCount = 0;
      }
      break;
    case 1:
      FIS_WRITE_line1 = "jjjjjjjj" +
        centerString8("RPM") +
        centerString8("COOLANT") +
        "jjjjjjjj";

      FIS_WRITE_line2 = "jjjjjjjj" +
        centerString8(String(rpm)) +
        centerString8(String(coolantTemp)+ "kC") +
        "jjjjjjjj";

      if (!reached100 && absSpeed_kmh < 1.0 && rpm > 4000){
        screen++;
      }

      break;
    case 2:
      FIS_WRITE_line1 = centerString8(String(accelTime)+ "S");
      FIS_WRITE_line2 = centerString8(String((uint8_t)(absSpeed_kmh + 0.5))+ "KM/H");

      if (reached100) {
        if (millis() > holdUntil) {
          reached100 = false;
          accelTime = 0;
          startTime = 0;
          screen--;
        }
      }
      else {
        if (absSpeed_kmh < 1.0f || millis() - startTime > 30000) {
          accelTime = 0;
          startTime = 0;
          screen--;
        }
      }

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

      if (!paused) {
        if (FIS_WRITE_rotary_position_line1 < FIS_WRITE_line1_length) {
          FIS_WRITE_rotary_position_line1++;
        }
        else {
          FIS_WRITE_rotary_position_line1 = 0;
        }
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

      if (!paused) {
        if (FIS_WRITE_rotary_position_line2 < FIS_WRITE_line2_length) {
          FIS_WRITE_rotary_position_line2++;
        }
        else {
          FIS_WRITE_rotary_position_line2 = 0;
        }

        // if went 8 symbols — run pause
        if ((FIS_WRITE_rotary_position_line2 % 8 == 0) &&
            FIS_WRITE_rotary_position_line2 > 0 &&
            FIS_WRITE_rotary_position_line2 < FIS_WRITE_line2_length - 8) 
        {
            paused = true;
            lastUpdate = millis();
        }
      }
      else {
        // wait
        if (millis() - lastUpdate >= delayTime) {
          paused = false;
        }
      }
    }
    else {
      smallStringCount++;
      FIS_WRITE_sendline2 = FIS_WRITE_line2;
    }

    //Serial.println("refresh");
    FIS_WRITE_sendTEXT(FIS_WRITE_sendline1, FIS_WRITE_sendline2);
    FIS_WRITE_last_refresh = millis();
  }

  //END WRITE TO CLUSTER
}

//WRITE TO CLUSTER
void FIS_WRITE_sendTEXT(String FIS_WRITE_line1, String FIS_WRITE_line2) {
  //Serial.print("|"); Serial.print(FIS_WRITE_line1); Serial.println("|");
  //Serial.print("|"); Serial.print(FIS_WRITE_line2); Serial.println("|");

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

void FIS_WRITE_startENA() {
  if (!digitalRead(FIS_WRITE_ENA)) {
    digitalWrite(FIS_WRITE_ENA, HIGH);
    //  delayMicroseconds(FIS_WRITE_STARTPULSEW);
    //  digitalWrite(FIS_WRITE_ENA,LOW);
    //  delayMicroseconds(FIS_WRITE_STARTPULSEW);
    //  digitalWrite(FIS_WRITE_ENA,HIGH);
    //  delayMicroseconds(FIS_WRITE_STARTPULSEW);
    // FIS_WRITE_ENA_STATUS=1;
  }
}

void FIS_WRITE_stopENA() {
  digitalWrite(FIS_WRITE_ENA, LOW);
  // FIS_WRITE_ENA_STATUS=0;
}
//END WRITE TO CLUSTER

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

