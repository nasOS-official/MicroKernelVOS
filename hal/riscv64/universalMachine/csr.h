#ifndef RISCV64_CSR_H
#define RISCV64_CSR_H

#include <stdint.h>

/* ==========================================================================
 * CSR Addresses
 * ========================================================================== */

/* Supervisor Mode CSRs */
#define CSR_SSTATUS     sstatus
#define CSR_SIE         sie
#define CSR_STVEC       stvec
#define CSR_SSCRATCH    sscratch
#define CSR_SEPC        sepc
#define CSR_SCAUSE      scause
#define CSR_STVAL       stval
#define CSR_SIP         sip
#define CSR_SATP        satp

/* Machine Mode CSRs */
#define CSR_MSTATUS     mstatus
#define CSR_MISA        misa
#define CSR_MEDELEG     medeleg
#define CSR_MIDELEG     mideleg
#define CSR_MIE         mie
#define CSR_MTVEC       mtvec
#define CSR_MSCRATCH    mscratch
#define CSR_MEPC        mepc
#define CSR_MCAUSE      mcause
#define CSR_MTVAL       mtval
#define CSR_MIP         mip

/* ==========================================================================
 * Bitfield Definitions
 * ========================================================================== */

/* --- SSTATUS (Supervisor Status) --- */
#define SSTATUS_SIE_SHIFT       1
#define SSTATUS_SIE             (1ULL << SSTATUS_SIE_SHIFT)     /* Supervisor Interrupt Enable */

#define SSTATUS_SPIE_SHIFT      5
#define SSTATUS_SPIE            (1ULL << SSTATUS_SPIE_SHIFT)    /* Supervisor Previous Interrupt Enable */

#define SSTATUS_SPP_SHIFT       8
#define SSTATUS_SPP             (1ULL << SSTATUS_SPP_SHIFT)     /* Supervisor Previous Privilege (0=U, 1=S) */

#define SSTATUS_FS_SHIFT        13
#define SSTATUS_FS_MASK         (3ULL << SSTATUS_FS_SHIFT)      /* Floating-Point Status */
#define SSTATUS_FS_OFF          (0ULL << SSTATUS_FS_SHIFT)
#define SSTATUS_FS_INITIAL      (1ULL << SSTATUS_FS_SHIFT)
#define SSTATUS_FS_CLEAN        (2ULL << SSTATUS_FS_SHIFT)
#define SSTATUS_FS_DIRTY        (3ULL << SSTATUS_FS_SHIFT)

#define SSTATUS_XS_SHIFT        15
#define SSTATUS_XS_MASK         (3ULL << SSTATUS_XS_SHIFT)      /* User Extensions Status */

#define SSTATUS_SUM_SHIFT       18
#define SSTATUS_SUM             (1ULL << SSTATUS_SUM_SHIFT)     /* Supervisor User Memory Access */

#define SSTATUS_MXR_SHIFT       19
#define SSTATUS_MXR             (1ULL << SSTATUS_MXR_SHIFT)     /* Make Executable Readable */

#define SSTATUS_UXL_SHIFT       32
#define SSTATUS_UXL_MASK        (3ULL << SSTATUS_UXL_SHIFT)     /* User XLEN (RV64) */
#define SSTATUS_UXL_64          (2ULL << SSTATUS_UXL_SHIFT)

#define SSTATUS_SD_SHIFT        63
#define SSTATUS_SD              (1ULL << SSTATUS_SD_SHIFT)      /* State Dirty (FS/XS dirty) */

/* --- MSTATUS (Machine Status + SSTATUS) --- */
#define MSTATUS_MIE_SHIFT       3
#define MSTATUS_MIE             (1ULL << MSTATUS_MIE_SHIFT)     /* Machine Interrupt Enable */

#define MSTATUS_MPIE_SHIFT      7
#define MSTATUS_MPIE            (1ULL << MSTATUS_MPIE_SHIFT)    /* Machine Previous Interrupt Enable */

#define MSTATUS_MPP_SHIFT       11
#define MSTATUS_MPP_MASK        (3ULL << MSTATUS_MPP_SHIFT)     /* Machine Previous Privilege */
#define MSTATUS_MPP_U           (0ULL << MSTATUS_MPP_SHIFT)
#define MSTATUS_MPP_S           (1ULL << MSTATUS_MPP_SHIFT)
#define MSTATUS_MPP_M           (3ULL << MSTATUS_MPP_SHIFT)

#define MSTATUS_MPRV_SHIFT      17
#define MSTATUS_MPRV            (1ULL << MSTATUS_MPRV_SHIFT)    /* Modify Privilege */

#define MSTATUS_TVM_SHIFT       20
#define MSTATUS_TVM             (1ULL << MSTATUS_TVM_SHIFT)     /* Trap Virtual Memory */

#define MSTATUS_TW_SHIFT        21
#define MSTATUS_TW              (1ULL << MSTATUS_TW_SHIFT)      /* Timeout Wait */

#define MSTATUS_TSR_SHIFT       22
#define MSTATUS_TSR             (1ULL << MSTATUS_TSR_SHIFT)     /* Trap SRET */

#define MSTATUS_SXL_SHIFT       34
#define MSTATUS_SXL_MASK        (3ULL << MSTATUS_SXL_SHIFT)     /* Supervisor XLEN (RV64) */

/* --- SIE / MIE (Interrupt Enable Registers) --- */
#define SIE_SSIE                (1ULL << 1)                     /* Supervisor Software Interrupt Enable */
#define SIE_STIE                (1ULL << 5)                     /* Supervisor Timer Interrupt Enable */
#define SIE_SEIE                (1ULL << 9)                     /* Supervisor External Interrupt Enable */

#define MIE_MSIE                (1ULL << 3)                     /* Machine Software Interrupt Enable */
#define MIE_MTIE                (1ULL << 7)                     /* Machine Timer Interrupt Enable */
#define MIE_MEIE                (1ULL << 11)                    /* Machine External Interrupt Enable */

/* --- SIP / MIP (Interrupt Pending Registers) --- */
#define SIP_SSIP                (1ULL << 1)                     /* Supervisor Software Interrupt Pending */
#define SIP_STIP                (1ULL << 5)                     /* Supervisor Timer Interrupt Pending */
#define SIP_SEIP                (1ULL << 9)                     /* Supervisor External Interrupt Pending */

#define MIP_MSIP                (1ULL << 3)
#define MIP_MTIP                (1ULL << 7)
#define MIP_MEIP                (1ULL << 11)

/* --- SATP (Supervisor Address Translation and Protection, RV64) --- */
#define SATP64_MODE_SHIFT       60
#define SATP64_MODE_MASK        (0xFULL << SATP64_MODE_SHIFT)
#define SATP64_MODE_BARE        (0ULL << SATP64_MODE_SHIFT)
#define SATP64_MODE_SV39        (8ULL << SATP64_MODE_SHIFT)
#define SATP64_MODE_SV48        (9ULL << SATP64_MODE_SHIFT)
#define SATP64_MODE_SV57        (10ULL << SATP64_MODE_SHIFT)

#define SATP64_ASID_SHIFT       44
#define SATP64_ASID_MASK        (0xFFFFULL << SATP64_ASID_SHIFT)

#define SATP64_PPN_SHIFT        0
#define SATP64_PPN_MASK         (0x00000FFFFFFFFFFFULL)

/* --- SCAUSE / MCAUSE --- */
#define CAUSE_INTERRUPT_FLAG    (1ULL << 63)
#define CAUSE_CODE_MASK         (0x7FFFFFFFFFFFFFFFULL)

/* Common Exception Codes */
#define EXCEPTION_MISALIGNED_FETCH      0
#define EXCEPTION_FETCH_ACCESS          1
#define EXCEPTION_ILLEGAL_INSTRUCTION   2
#define EXCEPTION_BREAKPOINT            3
#define EXCEPTION_MISALIGNED_LOAD       4
#define EXCEPTION_LOAD_ACCESS           5
#define EXCEPTION_MISALIGNED_STORE      6
#define EXCEPTION_STORE_ACCESS          7
#define EXCEPTION_USER_ECALL            8
#define EXCEPTION_SUPERVISOR_ECALL      9
#define EXCEPTION_MACHINE_ECALL         11
#define EXCEPTION_FETCH_PAGE_FAULT      12
#define EXCEPTION_LOAD_PAGE_FAULT       13
#define EXCEPTION_STORE_PAGE_FAULT      15

/* Common Interrupt Codes */
#define INTERRUPT_SUPERVISOR_SOFTWARE   1
#define INTERRUPT_MACHINE_SOFTWARE      3
#define INTERRUPT_SUPERVISOR_TIMER     5
#define INTERRUPT_MACHINE_TIMER        7
#define INTERRUPT_SUPERVISOR_EXTERNAL   9
#define INTERRUPT_MACHINE_EXTERNAL      11

/* --- STVEC / MTVEC --- */
#define TVEC_MODE_SHIFT         0
#define TVEC_MODE_MASK          (3ULL)
#define TVEC_MODE_DIRECT        (0ULL)
#define TVEC_MODE_VECTORED      (1ULL)
#define TVEC_BASE_MASK          (~3ULL)

#endif /* RISCV64_CSR_H */