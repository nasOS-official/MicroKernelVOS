#ifndef __CPUI_H
#define __CPUI_H

#include "cpu.h"
#include "tasks.h"
#include <stdint.h>
#define MAX_CPUS 1024

typedef struct cpu_core_t {
    uint32_t cpu_id;
    struct task_control_block_t* current_thread;

    struct task_control_block_t* ready_head[MAX_PRIORITY];
    struct task_control_block_t* ready_tail[MAX_PRIORITY];
    uint32_t ready_summary;

    uint32_t ready_count;
} cpu_core_t;

extern cpu_core_t* cpu_list[MAX_CPUS];

extern uint32_t core_count;


void ki_cpu_init();

#endif