// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __CPU_H
#define __CPU_H
#include <stdint.h>

void cpu_init(uint32_t cpu);
void syscall_init();

typedef struct cpu_core_t {
    uint32_t cpu_id;
    struct task_control_block_t* current_thread;
    struct cpu_core_t* next;
} cpu_core_t;

typedef struct syscall_frame {
    uint64_t syscall_num;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint64_t arg4;
    uint64_t arg5;
    uint64_t arg6;
} syscall_frame;


void set_thread_info_ptr(struct task_control_block_t* ptr);
struct task_control_block_t* get_thread_info_ptr();
void set_kernel_stack(void* kernel_SP);


#endif