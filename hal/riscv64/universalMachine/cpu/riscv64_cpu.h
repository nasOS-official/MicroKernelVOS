#ifndef __RISCV64_CPU_H
#define __RISCV64_CPU_H

#include <stdint.h>

typedef struct hw_data_t{
}  hw_data_t;

typedef struct hw_context_data_t {
    uintptr_t ra, sp, gp, tp, t0, t1, t2, s0, s1, a0, a1, a2, a3, a4, a5, a6, a7, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, t3, t4, t5, t6;
    uintptr_t sepc;
    uintptr_t sstatus;
    uintptr_t sscratch;
} hw_context_data_t;


#endif