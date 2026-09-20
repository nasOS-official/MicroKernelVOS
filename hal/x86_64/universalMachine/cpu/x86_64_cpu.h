#ifndef __X86_64_CPU_H
#define __X86_64_CPU_H

#include <stdint.h>

typedef struct hw_data_t{
    uint8_t ports[8192];
}  hw_data_t;


static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint64_t read_tsc(void) {
    uint32_t lo, hi;
    asm volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    ((void)lo);
    return ((uint64_t)hi << 32) | lo;
}

#endif