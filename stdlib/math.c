#include "math.h"
#include <stdint.h>


int pow(int num, int p) {
  if (p == 0) {
    return 1;
  } else if (p == 1 || num == 1) {
    return num;
  } else if (p % 2 == 1) {
    return num * pow(num, p - 1);
  } else {
    return pow(num * num, p / 2);
  }
}
