// SPDX-License-Identifier: GPL-2.0-or-later
#include "stdbool.h"
#include "debug.h"
#include <panic.h>

void hcf(void) {
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__) || defined (__riscv)
        asm ("wfi");
#elif defined (__loongarch64)
        asm ("idle 0");
#endif
    }
}


void panic(){
    logk("KERNEL PANIC");
    hcf();
}