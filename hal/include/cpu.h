// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __CPU_H
#define __CPU_H
#include <stdint.h>


#if defined(__x86_64__)
#include "../x86_64/universalMachine/cpu/x86_64_cpu.h"
#endif

#if defined(__riscv) && (__riscv_xlen == 64)
#include "../riscv64/universalMachine/cpu/riscv64_cpu.h"
#endif

typedef struct syscall_frame {
    uint64_t syscall_num;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint64_t arg4;
    uint64_t arg5;
    uint64_t arg6;
} syscall_frame;

void cpu_init(uint32_t cpu);
int64_t syscall_handler(struct syscall_frame* regs);

typedef struct cpu_core_t {
    uint32_t cpu_id;
    struct task_control_block_t* current_thread;
    struct cpu_core_t* next;
} cpu_core_t;




void set_thread_info_ptr(struct task_control_block_t* ptr);
struct task_control_block_t* get_thread_info_ptr();
void set_kernel_stack(void* kernel_SP);

void start_timer(uint64_t us);
void prepare_stack(struct task_control_block_t *task);
void userspace_jmp(struct task_control_block_t *task);


#endif