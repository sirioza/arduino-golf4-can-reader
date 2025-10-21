#include <utils.h>

const int points = 53;
uint16_t rpmTable[points] = {
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