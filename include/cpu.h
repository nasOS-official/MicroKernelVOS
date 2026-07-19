// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __CPU_H
#define __CPU_H

#include <stdint.h>

typedef struct cpu_core_t {
    uint32_t cpu_id;
    struct task_control_block_t* current_thread;
    struct cpu_core_t* next;
} cpu_core_t;


void cpu_init();


#endif