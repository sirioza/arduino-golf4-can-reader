#pragma once
#include <Arduino.h>
#include <mcp_can.h>

extern MCP_CAN CAN;

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

void CAN_Init(byte attempts);
bool CAN_HasMessage();
void CAN_ReadMessage(uint32_t &id, byte &len, byte *buf);
