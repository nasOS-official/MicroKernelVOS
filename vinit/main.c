#include <stdint.h>

#if defined (__x86_64__)

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


#elif defined (__riscv)

static inline int64_t syscall6(
    int64_t nr,
    int64_t arg1,
    int64_t arg2,
    int64_t arg3,
    int64_t arg4,
    int64_t arg5,
    int64_t arg6)
{
    register int64_t a0 __asm__("a0") = arg1;
    register int64_t a1 __asm__("a1") = arg2;
    register int64_t a2 __asm__("a2") = arg3;
    register int64_t a3 __asm__("a3") = arg4;
    register int64_t a4 __asm__("a4") = arg5;
    register int64_t a5 __asm__("a5") = arg6;
    register int64_t a7 __asm__("a7") = nr;

    __asm__ volatile (
        "ecall"
        : "+r" (a0)
        : "r" (a1), "r" (a2), "r" (a3), "r" (a4), "r" (a5), "r" (a7)
        : "memory"
    );

    return a0;
}
#endif

// static unsigned long int next = 1;

// int rand(void)
// {
//     next = next * 1103515245 + 12345;
//     return (unsigned int)(next/65536) % 32768;
// }

// void srand(unsigned int seed)
// {
//     next = seed;
// }
void thread2(void){
    char* string = "thread2 print";
    volatile uint64_t a = 0;
    for(;;){
        syscall6(993, (int64_t)string, 0, 0, 0, 0, 0);

        // for (uint64_t x = 0; x < 1000*4096; x++){
        //     *((uint8_t*)addr + x) = (uint8_t)rand();
        // }
        a++;
    }
}

void _start(void){

    void* addr = (void*)syscall6(31, 0x1000000000, 10, 0, 0, 0, 0);
    char* string = "thread1 print";
    syscall6(90, 1, (uint64_t)&thread2, (uint64_t)addr + 0x2000, 0, 0, 0);


    volatile uint64_t a = 0;
    for(;;){
        syscall6(993, (int64_t)string, 0, 0, 0, 0, 0);

        // for (uint64_t x = 0; x < 1000*4096; x++){
        //     *((uint8_t*)addr + x) = (uint8_t)rand();
        // }
        a++;
    }
}