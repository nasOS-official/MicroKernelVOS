#include "limine_requests.h"
#include "stdbool.h"
#include "../debug/fb.h"
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
    if (framebuffer_request.response->framebuffer_count > 0){
        log("KERNEL PANIC", framebuffer_request.response->framebuffers[0]);
    }
    hcf();
}