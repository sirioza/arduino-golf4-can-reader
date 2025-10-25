#include "vehicle_utils.h"

const int points = 31;
uint16_t rpmTable[points] = {
  0, 800, 1000, 1200, 1400, 1600, 1800, 2000, 2200, 2400, 2600,
  2800, 3000, 3200, 3400, 3600, 3800, 4000, 4200, 4400, 4600,
  4800, 5000, 5200, 5400, 5600, 5800, 6000, 6200, 6400, 6500
};
float torqueTable[points] = {
  0, 106, 114, 122, 130, 137, 143, 148, 148, 148, 150, 
  154, 158, 162, 166, 170, 174, 174, 174, 170, 164,
  161, 159, 160, 160, 156, 151, 146, 141, 137, 136
};

int16_t getCoolantTemp(uint8_t AA) {
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

float getTorque(uint16_t  rpm) {
  if (rpm <= rpmTable[0]){
    return torqueTable[0];
  }

  if (rpm >= rpmTable[points-1]){
    return torqueTable[points-1];
  }

  for (int8_t i = 0; i < points - 1; i++) {
    if (rpm >= rpmTable[i] && rpm <= rpmTable[i + 1]) {
      float torque = (rpm - rpmTable[i]) / (rpmTable[i + 1] - rpmTable[i]);

      return torqueTable[i] + torque * (torqueTable[i + 1] - torqueTable[i]);
    }
  }

  return torqueTable[points-1];
}