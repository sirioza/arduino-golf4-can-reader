#include "vehicle_utils.h"

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