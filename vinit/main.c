#include <stdint.h>
static inline int64_t syscall6(
    int64_t nr,
    int64_t arg1,
    int64_t arg2,
    int64_t arg3,
    int64_t arg4,
    int64_t arg5,
    int64_t arg6)
{
    int64_t ret;

    register int64_t r10 __asm__("r10") = arg4;
    register int64_t r8  __asm__("r8")  = arg5;
    register int64_t r9  __asm__("r9")  = arg6;

    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(nr),
          "D"(arg1),
          "S"(arg2),
          "d"(arg3),
          "r"(r10),
          "r"(r8),
          "r"(r9)
        : "rcx", "r11", "memory"
    );

    return ret;
}


static unsigned long int next = 1;

int rand(void)
{
    next = next * 1103515245 + 12345;
    return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed)
{
    next = seed;
}


void _start(void){

    void* addr = (void*)syscall6(30, 0x1000000000, 1000, 0xfd000000 >> 12, 7, 0, 0);

    for(;;){
        for (uint64_t x = 0; x < 1000*4096; x++){
            *((uint8_t*)addr + x) = (uint8_t)rand();
        }
    }
}