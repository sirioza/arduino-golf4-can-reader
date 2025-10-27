//CONTROL UNIT PIN
#define FIS_WRITE_ENA 2
#define FIS_WRITE_CLK 3
#define FIS_WRITE_DATA 4
#define FIS_WRITE_PULSEW 50
#define FIS_WRITE_STARTPULSEW 100
#define FIS_WRITE_START 15 //something like address, first byte is always 15
#define REFRESH_CLASTER_TIME 300

//CAN PIN
#define SPI_CS_PIN 10   // CS
#define CAN_INT_PIN 9   // INT

//ENCODER PIN
#define ENCODER_CLK 6
#define ENCODER_DT 7
#define ENC_BTN_SW 8

//CLUSTERS ID
#define SPEEDOMETR_ID 0x5A0
#define COOLANT_TEMP_ID 0x288
#define RPM_ID 0x280
#define ABS_SPEED_ID 0x1A0
#define NO_FOUND1_ID 0x488
#define NO_FOUND2_ID 0x320
#define NO_FOUND3_ID 0x50
#define NO_FOUND4_ID 0x5D0
#define NO_FOUND5_ID 0x420
#define NO_FOUND6_ID 0x4A0

