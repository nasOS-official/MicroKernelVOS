#ifndef __SBI_H
#define __SBI_H

#include <stdint.h>

struct sbiret {
    long error;
    long value;
};

/* =========================================================================
 * Error Codes
 * ========================================================================= */

#define SBI_SUCCESS                  0
#define SBI_ERR_FAILED              -1
#define SBI_ERR_NOT_SUPPORTED       -2
#define SBI_ERR_INVALID_PARAM       -3
#define SBI_ERR_DENIED              -4
#define SBI_ERR_INVALID_ADDRESS     -5
#define SBI_ERR_ALREADY_AVAILABLE   -6
#define SBI_ERR_ALREADY_STARTED     -7
#define SBI_ERR_ALREADY_STOPPED     -8

/* =========================================================================
 * Legacy Extension IDs (EID)
 * Note: fid = 0
 * ========================================================================= */

#define SBI_SET_TIMER               0x00
#define SBI_CONSOLE_PUTCHAR         0x01
#define SBI_CONSOLE_GETCHAR         0x02
#define SBI_CLEAR_IPI               0x03
#define SBI_SEND_IPI                0x04
#define SBI_REMOTE_FENCE_I          0x05
#define SBI_REMOTE_SFENCE_VMA       0x06
#define SBI_REMOTE_SFENCE_VMA_ASID  0x07
#define SBI_SHUTDOWN                0x08

/* =========================================================================
 * Standard Extension IDs (EID) и Function IDs (FID) — SBI v0.2+
 * ========================================================================= */

/* --- Base Extension (EID: 0x10) --- */
#define SBI_EXT_BASE                0x10
#define SBI_EXT_BASE_GET_SPEC_VERSION       0x0
#define SBI_EXT_BASE_GET_IMP_ID             0x1
#define SBI_EXT_BASE_GET_IMP_VERSION        0x2
#define SBI_EXT_BASE_PROBE_EXT              0x3
#define SBI_EXT_BASE_GET_MVENDORID          0x4
#define SBI_EXT_BASE_GET_MARCHID            0x5
#define SBI_EXT_BASE_GET_MIMPID             0x6

/* --- Timer Extension (EID: 0x54494D45 "TIME") --- */
#define SBI_EXT_TIME                0x54494D45
#define SBI_EXT_TIME_SET_TIMER              0x0

/* --- IPI Extension (EID: 0x735049 "sPI") --- */
#define SBI_EXT_IPI                 0x735049
#define SBI_EXT_IPI_SEND_IPI                0x0

/* --- RFENCE Extension (EID: 0x52464E43 "RFNC") --- */
#define SBI_EXT_RFENCE              0x52464E43
#define SBI_EXT_RFENCE_REMOTE_FENCE_I       0x0
#define SBI_EXT_RFENCE_REMOTE_SFENCE_VMA    0x1
#define SBI_EXT_RFENCE_REMOTE_SFENCE_VMA_ASID 0x2
#define SBI_EXT_RFENCE_REMOTE_HFENCE_GVMA_VMID 0x3
#define SBI_EXT_RFENCE_REMOTE_HFENCE_GVMA   0x4
#define SBI_EXT_RFENCE_REMOTE_HFENCE_VVMA_ASID 0x5
#define SBI_EXT_RFENCE_REMOTE_HFENCE_VVMA   0x6

/* --- Hart State Management Extension (EID: 0x48534D "HSM") --- */
#define SBI_EXT_HSM                 0x48534D
#define SBI_EXT_HSM_HART_START              0x0
#define SBI_EXT_HSM_HART_STOP               0x1
#define SBI_EXT_HSM_HART_GET_STATUS         0x2
#define SBI_EXT_HSM_HART_SUSPEND            0x3

/* --- System Reset Extension (EID: 0x53525354 "SRST") --- */
#define SBI_EXT_SRST                0x53525354
#define SBI_EXT_SRST_SYSTEM_RESET           0x0

/* Reset_type */
#define SBI_SRST_RESET_TYPE_SHUTDOWN        0x0
#define SBI_SRST_RESET_TYPE_COLD_REBOOT     0x1
#define SBI_SRST_RESET_TYPE_WARM_REBOOT     0x2

/* Reset_reason */
#define SBI_SRST_RESET_REASON_NONE          0x0
#define SBI_SRST_RESET_REASON_SYSTEM_FAILURE 0x1

/* --- Debug Console Extension (EID: 0x4442434E "DBCN") --- */
#define SBI_EXT_DBCN                0x4442434E
#define SBI_EXT_DBCN_CONSOLE_WRITE          0x0
#define SBI_EXT_DBCN_CONSOLE_READ           0x1
#define SBI_EXT_DBCN_CONSOLE_WRITE_BYTE     0x2

/* --- Performance Monitoring Unit Extension (EID: 0x504D55 "PMU") --- */
#define SBI_EXT_PMU                 0x504D55
#define SBI_EXT_PMU_NUM_COUNTERS            0x0
#define SBI_EXT_PMU_COUNTER_GET_INFO        0x1
#define SBI_EXT_PMU_COUNTER_CONFIG_MATCHING 0x2
#define SBI_EXT_PMU_COUNTER_START           0x3
#define SBI_EXT_PMU_COUNTER_STOP            0x4
#define SBI_EXT_PMU_COUNTER_FW_READ         0x5
#define SBI_EXT_PMU_COUNTER_FW_READ_RAW     0x6

/* --- System Suspend Extension (EID: 0x53555350 "SUSP") --- */
#define SBI_EXT_SUSP                0x53555350
#define SBI_EXT_SUSP_SUSPEND                0x0

/* --- Vector Reset Extension (EID: 0x43565452 "CVTR") --- */
#define SBI_EXT_CPPC                0x43505043
#define SBI_EXT_CPPC_PROBE                  0x0
#define SBI_EXT_CPPC_READ                   0x1
#define SBI_EXT_CPPC_WRITE                  0x2

static inline struct sbiret sbi_ecall(int ext, int fid, unsigned long arg0,
                                     unsigned long arg1, unsigned long arg2,
                                     unsigned long arg3, unsigned long arg4,
                                     unsigned long arg5) {
    struct sbiret ret;
    register unsigned long a0 __asm__("a0") = arg0;
    register unsigned long a1 __asm__("a1") = arg1;
    register unsigned long a2 __asm__("a2") = arg2;
    register unsigned long a3 __asm__("a3") = arg3;
    register unsigned long a4 __asm__("a4") = arg4;
    register unsigned long a5 __asm__("a5") = arg5;
    register unsigned long a6 __asm__("a6") = fid;
    register unsigned long a7 __asm__("a7") = ext;

    __asm__ __volatile__(
        "ecall"
        : "+r"(a0), "+r"(a1)
        : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
        : "memory"
    );

    ret.error = a0;
    ret.value = a1;
    return ret;
}

#endif