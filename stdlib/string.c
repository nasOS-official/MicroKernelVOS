#include "string.h"
#include <stdint.h>

void *
memmove (void *dest, const void *src, size_t len)
{
  char *d = dest;
  const char *s = src;
  if (d < s)
    while (len--)
      *d++ = *s++;
  else
    {
      char *lasts = (char *)s + (len-1);
      char *lastd = d + (len-1);
      while (len--)
        *lastd-- = *lasts--;
    }
  return dest;



}

void *memset(void *str, int c, unsigned long n) {
    char *destcpy;
    destcpy = (char *)str;
    while (n > 0) {
        *destcpy = (char)c;
        destcpy++;
        n--;
    }
    return (str);
}


size_t strlen(const char *str)
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}




void *memcpy(void *dest, const void *src, size_t n)
{
    char *d = (char *)dest;
    const char *s = (const char *)src;

    if (n == 0 || dest == src)
        return dest;

    if (n >= sizeof(uintptr_t)) {
        while ((uintptr_t)d % sizeof(uintptr_t) != 0 && n > 0) {
            *d++ = *s++;
            --n;
        }

        for (; n >= sizeof(uintptr_t); n -= sizeof(uintptr_t)) {
            *(uintptr_t*)d = *(const uintptr_t*)s;
            d += sizeof(uintptr_t);
            s += sizeof(uintptr_t);
        }
    }

    while (n > 0) {
        *d++ = *s++;
        --n;
    }

    return dest;
}

char *
strcpy(char* dest, const char* src)
{
	memcpy(dest, src, strlen(src) + 1);
	return dest;
}

char *
strcat (char *dest, const char *src)
{
  strcpy (dest + strlen (dest), src);
  return dest;
}