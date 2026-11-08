#include <stddef.h>
#include <stdint.h>


#include "tasks.h"

#define DEFINE_OFFSET(sym, str, mem) \
    __asm__(".equ " #sym ", %0" : : "i"(offsetof(str, mem)))

#define DEFINE_SIZE(sym, str) \
    __asm__(".equ " #sym ", %0" : : "i"(sizeof(str)))

void generate_offsets(void) {
    DEFINE_OFFSET(CTX_RA,       hw_context_data_t, ra);
    DEFINE_OFFSET(CTX_SP,       hw_context_data_t, sp);
    DEFINE_OFFSET(CTX_GP,       hw_context_data_t, gp);
    DEFINE_OFFSET(CTX_TP,       hw_context_data_t, tp);
    DEFINE_OFFSET(CTX_T0,       hw_context_data_t, t0);
    DEFINE_OFFSET(CTX_T1,       hw_context_data_t, t1);
    DEFINE_OFFSET(CTX_T2,       hw_context_data_t, t2);
    DEFINE_OFFSET(CTX_S0,       hw_context_data_t, s0);
    DEFINE_OFFSET(CTX_S1,       hw_context_data_t, s1);
    DEFINE_OFFSET(CTX_A0,       hw_context_data_t, a0);
    DEFINE_OFFSET(CTX_A1,       hw_context_data_t, a1);
    DEFINE_OFFSET(CTX_A2,       hw_context_data_t, a2);
    DEFINE_OFFSET(CTX_A3,       hw_context_data_t, a3);
    DEFINE_OFFSET(CTX_A4,       hw_context_data_t, a4);
    DEFINE_OFFSET(CTX_A5,       hw_context_data_t, a5);
    DEFINE_OFFSET(CTX_A6,       hw_context_data_t, a6);
    DEFINE_OFFSET(CTX_A7,       hw_context_data_t, a7);
    DEFINE_OFFSET(CTX_S2,       hw_context_data_t, s2);
    DEFINE_OFFSET(CTX_S3,       hw_context_data_t, s3);
    DEFINE_OFFSET(CTX_S4,       hw_context_data_t, s4);
    DEFINE_OFFSET(CTX_S5,       hw_context_data_t, s5);
    DEFINE_OFFSET(CTX_S6,       hw_context_data_t, s6);
    DEFINE_OFFSET(CTX_S7,       hw_context_data_t, s7);
    DEFINE_OFFSET(CTX_S8,       hw_context_data_t, s8);
    DEFINE_OFFSET(CTX_S9,       hw_context_data_t, s9);
    DEFINE_OFFSET(CTX_S10,      hw_context_data_t, s10);
    DEFINE_OFFSET(CTX_S11,      hw_context_data_t, s11);
    DEFINE_OFFSET(CTX_T3,       hw_context_data_t, t3);
    DEFINE_OFFSET(CTX_T4,       hw_context_data_t, t4);
    DEFINE_OFFSET(CTX_T5,       hw_context_data_t, t5);
    DEFINE_OFFSET(CTX_T6,       hw_context_data_t, t6);
    
    DEFINE_OFFSET(CTX_SEPC,     hw_context_data_t, sepc);
    DEFINE_OFFSET(CTX_SSTATUS,  hw_context_data_t, sstatus);
    DEFINE_OFFSET(CTX_SSCRATCH, hw_context_data_t, sscratch);
    
    DEFINE_SIZE(CTX_SIZE,       hw_context_data_t);

    DEFINE_OFFSET(TCB_KERNEL_SP, task_control_block_t, kernel_SP);
    DEFINE_OFFSET(TCB_USER_SP,   task_control_block_t, user_SP);
}