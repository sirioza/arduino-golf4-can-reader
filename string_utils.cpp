#include <string_utils.h>

String centerString8(const String& input) {
    String result = "        "; // 8 whitespace
    int8_t len = input.length();
    if (len >= 8) {
        result = input.substring(0, 8);
    }
    else {
      int8_t totalPadding = 8 - len;
      int8_t paddingLeft, paddingRight;

      if (len % 2 == 0) {
          paddingLeft = totalPadding / 2;
          paddingRight = totalPadding - paddingLeft;
      } else {
          paddingLeft = totalPadding / 2 + 1;
          paddingRight = totalPadding - paddingLeft;
      }

      int8_t index = 0;
      for (int8_t i = 0; i < paddingLeft; i++){
        result[index++] = ' ';
      }

      for (int8_t i = 0; i < len; i++){
        result[index++] = input[i];
      }

      for (int8_t i = 0; i < paddingRight; i++){
        result[index++] = ' ';
      }
    }

    return result;
}