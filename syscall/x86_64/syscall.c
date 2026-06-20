
#include "paging.h"
#include "process.h"
#include "msr.h"
#include <stdint.h>
#include <syscall.h>


extern void syscall_entry(void);

int64_t syscall_handler(uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5){
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;

    switch (arg1) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            return -1;
            break;
        case 8:
            return (int64_t)alloc_user_pages(arg2, arg3, arg4);
        case 9:
            return free_user_pages(arg2, arg3, arg4);
        default:
            return -1;
            break;
    }
}

// Write to MSR
void wrmsr(uint32_t addr, uint64_t data){
    const uint32_t eax = (uint64_t)(data) & 0xFFFFFFFF;
    const uint32_t edx = (uint64_t)(data) >> 32;
    asm volatile(
        "wrmsr"
        :
        : "c"(addr), "a"(eax), "d"(edx)
        : "memory");
}


void syscall_init(){
    wrmsr(IA32_LSTAR, (uint64_t)&syscall_entry);
}
