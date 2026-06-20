#ifndef __FB_H
#define __FB_H

#include <stdint.h>
#include <limine.h>

extern const uint8_t vgafont16[256][16];

void log(char* string, struct limine_framebuffer *framebuffer);
#endif