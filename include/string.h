#ifndef __STRING_H
#define __STRING_H
#include "stdint.h"
#include "stddef.h"

void* memmove (void *dest, const void *src, size_t len);

void* memset(void *str, int c, unsigned long n);

size_t strlen(const char *str);

void *memcpy(void *dest, const void *src, size_t n);

char* strcpy(char* dest, const char* src);

char* strcat (char *dest, const char *src);

#endif