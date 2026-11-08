// SPDX-License-Identifier: GPL-2.0-or-later
#include <stddef.h>
#include <paging.h>
#include <cpu.h>
#include <stdint.h>
#include "cpui.h"

uint32_t core_count = 0;

cpu_core_t* cpu_list[MAX_CPUS];


void ki_cpu_init(){
    cpu_list[0] = kernel_zalloc(sizeof(cpu_core_t));
    cpu_list[0]->cpu_id = 0;
    cpu_list[0]->current_thread = NULL;
    core_count = 1;
    cpu_init(0);
}

cpu_core_t* get_current_cpu(){
    return cpu_list[get_thread_info_ptr()->cpu_id];
}