// SPDX-License-Identifier: GPL-2.0-or-later
#include "stdlib.h"
#include "string.h"
#include "stdint.h"

void reverse(char s[])
{
	int i, j;
	char c;

	for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
	{
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

void itoa(int32_t n, char s[])
{
	int i, sign;

	if ((sign = n) < 0) 
		n = -n;	
	i = 0;
	do
	{						   
		s[i++] = n % 10 + '0'; 
	} while ((n /= 10) > 0); 
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
}

void uitoa(uint32_t n, char s[])
{
	int i;
	i = 0;
	do
	{						   
		s[i++] = n % 10 + '0'; 
	} while ((n /= 10) > 0); 
	s[i] = '\0';
	reverse(s);
}

static uint32_t next = 1;

int rand() {
    next = next * 1103515245 + 12345;
    return (uint32_t)(next / 65536) % 32768;
}

void srand(uint32_t seed) {
    next = seed;
}