#include "panic.h"
#include "riscv64_cpu.h"
#include "stddef.h"
#include <snprintf.h>
#include <stdint.h>
#include <string.h>
#include "cpu.h"
#include "tasks.h"
#include "debug.h"
#include "../csr.h"
#include "../sbi.h"


extern void trap_vector(void);

void cpu_init(uint32_t cpu){
    (void)cpu;
    __asm__ __volatile__("csrs sstatus, %0" :: "r"(SSTATUS_SUM) : "memory");
    uintptr_t addr = (uintptr_t)trap_vector & ~0x3ULL;
    __asm__ __volatile__("csrw stvec, %0" :: "r"(addr));
    __asm__ __volatile__ ("csrs sie, %0" : : "r" (SIE_STIE));
    __asm__ __volatile__ ("csrc sstatus, %0" : : "r" (SSTATUS_SIE));
}

extern struct task_control_block_t* sched_handler();


struct hw_context_data_t* rv64_ctrap_handler(struct hw_context_data_t* context){
    uintptr_t scause;
    uintptr_t stval;
    __asm__ __volatile__ (  "csrr %[SCAUSE], scause \n\t"
                            "csrr %[STVAL], stval \n\t"
                            : [SCAUSE] "=r"(scause),
                              [STVAL]  "=r"(stval)
                            :
                            : "memory");



    if (scause & 0x8000000000000000){
        logh("interrupt");
        switch (scause & 0x7FFFFFFFFFFFFFFF) {
            case 1:
                break;
            case 5:
                {
                    struct task_control_block_t *task = sched_handler();
                    context = (hw_context_data_t*)(task->kernel_SP);
                    break;
                }
            case 9:
                break;
        }
    } else {
        switch (scause & 0x7FFFFFFFFFFFFFFF) {
            case 8:
                {   
                    syscall_frame frame = {
                        .syscall_num = context->a7,
                        .arg1 = context->a0,
                        .arg2 = context->a1,
                        .arg3 = context->a2,
                        .arg4 = context->a3,
                        .arg5 = context->a4,
                        .arg6 = context->a5,
                    };

                    context->sepc += 4;
                    syscall_ret res = syscall_handler(&frame);
                    context->a0 = res.result;
                    struct task_control_block_t *task = res.task;
                    context = (hw_context_data_t*)(task->kernel_SP);
                    break;
                }
            default:
                {
                    logh("EXCEPTION_UNKNOWN");
                    char text[30];

                    snprintf(text, sizeof(text), "%llx", scause);
                    logh("SCAUSE");
                    logh(text);

                    snprintf(text, sizeof(text), "%llx", stval);
                    logh("STVAL");
                    logh(text);

                    snprintf(text, sizeof(text), "%llx", context->sepc);
                    logh("SEPC");
                    logh(text);
                    hcf();
                }
        }
    }

    
    return context;
}

void prepare_stack(struct task_control_block_t *task){
    uintptr_t sstat;
    asm volatile (  "csrr %[SSTAT], sstatus \n\t" 
                    : [SSTAT] "=r"(sstat) 
                    :
                    : "memory");
    task->kernel_SP -= sizeof(hw_context_data_t);
    hw_context_data_t *data = (hw_context_data_t*)(task->kernel_SP);
    memset(data, 0, sizeof(hw_context_data_t));
    data->sp = task->user_SP;
    data->tp = (uintptr_t)task;
    data->sscratch = 0;
    data->sepc = task->user_IP;
    data->sstatus = ((sstat | SSTATUS_SPIE | SSTATUS_SUM) & ~SSTATUS_SPP);

}

void start_timer(uint64_t us){
    uint64_t time;
    uint64_t us1 = 10000000 / 1000000;
    __asm__ __volatile__ ("rdtime %0" : "=r" (time));
    sbi_ecall(
        SBI_EXT_TIME, 
        SBI_EXT_TIME_SET_TIMER, 
        (unsigned long)time + us1 * us, 
        0, 0, 0, 0, 0
    );
}

void userspace_jmp(struct task_control_block_t *task){
    asm volatile (  "mv sp, %[SP] \n\t"
                    "j context_restore_normal \n\t" 
                    : 
                    : [SP] "r"(task->kernel_SP)
                    : "memory");
}

void set_thread_info_ptr(struct task_control_block_t* ptr){
    asm volatile ("mv tp, %[TP] \n\t"                
                : 
                : [TP] "r"(ptr)
                : "memory");
}

struct task_control_block_t* get_thread_info_ptr(){
    uintptr_t addr = 0;
    asm volatile ("mv %[TP], tp \n\t"                
            : [TP] "=r"(addr)
            : 
            : "memory");
    return (struct task_control_block_t*)addr;
}

void set_kernel_stack(void* kernel_SP){
    (void)kernel_SP;
}