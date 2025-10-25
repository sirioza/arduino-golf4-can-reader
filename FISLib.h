#include "Arduino.h"

#ifndef FISLib_h
#define FISLib_h

#define FIS_WRITE_PULSEW 50
#define FIS_WRITE_STARTPULSEW 100
#define FIS_WRITE_START 15 //something like address, first byte is always 15
#define REFRESH_CLUSTER_TIME 300

class FISLib{
  public:
    FISLib(uint8_t ena_pin, uint8_t clk_pin, uint8_t data_pin);
    void showText(String line1, String line2, uint16_t &smallStringCount);
    void sendTEXT(String line1, String line2);
    void sendByte(int8_t byte);
    void startENA();
    void stopENA();
  private:
    uint8_t _PIN_WRITE_ENA;
    uint8_t _PIN_WRITE_CLK;
    uint8_t _PIN_WRITE_DATA;
};

#endif