#ifndef __STDLIB_H
#define __STDLIB_H
#include "stdint.h"

void itoa(int32_t n, char s[]);

int rand();

void srand(uint32_t seed);

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))


#endif