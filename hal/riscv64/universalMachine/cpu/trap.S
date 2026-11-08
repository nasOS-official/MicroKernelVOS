#include "asm_offsets.h"

.section .text
.balign 4
.global trap_vector
.global context_restore_normal

.macro PUSH_CONTEXT
    addi sp, sp, -CTX_SIZE    
    sd x1, CTX_RA(sp)
    ld x1, TCB_USER_SP(tp)
    sd x1, CTX_SP(sp)
    sd x3, CTX_GP(sp)
    sd x4, CTX_TP(sp)
    sd x5, CTX_T0(sp)
    sd x6, CTX_T1(sp)
    sd x7, CTX_T2(sp)
    sd x8, CTX_S0(sp)
    sd x9, CTX_S1(sp)
    sd x10, CTX_A0(sp)
    sd x11, CTX_A1(sp)
    sd x12, CTX_A2(sp)
    sd x13, CTX_A3(sp)
    sd x14, CTX_A4(sp)
    sd x15, CTX_A5(sp)
    sd x16, CTX_A6(sp)
    sd x17, CTX_A7(sp)
    sd x18, CTX_S2(sp)
    sd x19, CTX_S3(sp)
    sd x20, CTX_S4(sp)
    sd x21, CTX_S5(sp)
    sd x22, CTX_S6(sp)
    sd x23, CTX_S7(sp)
    sd x24, CTX_S8(sp)
    sd x25, CTX_S9(sp)
    sd x26, CTX_S10(sp)
    sd x27, CTX_S11(sp)
    sd x28, CTX_T3(sp)
    sd x29, CTX_T4(sp)
    sd x30, CTX_T5(sp)
    sd x31, CTX_T6(sp)
    csrr t0, sepc
    sd t0, CTX_SEPC(sp)
    csrr t0, sstatus
    sd t0, CTX_SSTATUS(sp)
    csrr t0, sscratch
    sd t0, CTX_SSCRATCH(sp)
    sd sp, TCB_KERNEL_SP(tp)
.endm


.macro POP_CONTEXT
    ld t0, TCB_KERNEL_SP(tp)
    addi t0, t0, CTX_SIZE
    sd t0, TCB_KERNEL_SP(tp)

    ld t0, CTX_SEPC(sp)
    csrw sepc, t0
    ld t0, CTX_SSTATUS(sp)
    csrw sstatus, t0
    ld t0, CTX_SSCRATCH(sp)
    csrw sscratch, t0

    ld x1, CTX_RA(sp)
    ld x3, CTX_GP(sp)
    ld x4, CTX_TP(sp)
    ld x5, CTX_T0(sp)
    ld x6, CTX_T1(sp)
    ld x7, CTX_T2(sp)
    ld x8, CTX_S0(sp)
    ld x9, CTX_S1(sp)
    ld x10, CTX_A0(sp)
    ld x11, CTX_A1(sp)
    ld x12, CTX_A2(sp)
    ld x13, CTX_A3(sp)
    ld x14, CTX_A4(sp)
    ld x15, CTX_A5(sp)
    ld x16, CTX_A6(sp)
    ld x17, CTX_A7(sp)
    ld x18, CTX_S2(sp)
    ld x19, CTX_S3(sp)
    ld x20, CTX_S4(sp)
    ld x21, CTX_S5(sp)
    ld x22, CTX_S6(sp)
    ld x23, CTX_S7(sp)
    ld x24, CTX_S8(sp)
    ld x25, CTX_S9(sp)
    ld x26, CTX_S10(sp)
    ld x27, CTX_S11(sp)
    ld x28, CTX_T3(sp)
    ld x29, CTX_T4(sp)
    ld x30, CTX_T5(sp)
    ld x31, CTX_T6(sp)
    ld x2, CTX_SP(sp)
.endm

trap_vector:
    addi sp, sp, -16
    sd t0, 0(sp)
    csrr t0, sstatus
    andi t0, t0, 0x100
    bnez t0, nested_interrupt

normal_interrupt:

    ld t0, 0(sp)
    addi sp, sp, 16    

    csrrw tp, sscratch, tp

    sd sp, TCB_USER_SP(tp) 
    ld sp, TCB_KERNEL_SP(tp)
    j save_context_normal

nested_interrupt:
    ld t0, 0(sp)
    addi sp, sp, 16  
    sd sp, TCB_USER_SP(tp) 
    j save_context_nested

save_context_normal:
    PUSH_CONTEXT
exec_handler_normal:
    // передаем стек в аргументы
    mv a0, sp
    jal ra, rv64_ctrap_handler
    mv sp, a0

context_restore_normal:
    POP_CONTEXT

    csrrw tp, sscratch, tp

    sret
    
save_context_nested:
    PUSH_CONTEXT


exec_handler_nested:
    // передаем стек в аргументы
    mv a0, sp
    jal ra, rv64_ctrap_handler
    mv sp, a0

context_restore_nested:
    POP_CONTEXT

    sret