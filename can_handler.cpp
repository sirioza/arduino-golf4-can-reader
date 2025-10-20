#include "can_handler.h"
#include <SPI.h>
#include <config.h>

MCP_CAN CAN(SPI_CS_PIN);
bool isCanOk = false;
unsigned long cluster = 0;

void CAN_Init(byte attempts) {
    for (byte i = 0; i < attempts; i++) {
        isCanOk = CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK;
        if (isCanOk) {
            CAN.setMode(MCP_NORMAL);
            break;
        }
        delay(1000);
    }
}

bool CAN_HasMessage() {
    return isCanOk && CAN.checkReceive() == CAN_MSGAVAIL;
}

void CAN_ReadMessage(unsigned long &id, byte &len, byte *buf) {
    CAN.readMsgBuf(&id, &len, buf);
}