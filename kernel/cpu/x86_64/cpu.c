// SPDX-License-Identifier: GPL-2.0-or-later
#include <stddef.h>
#include <paging.h>
#include <cpu.h>
#include <stdint.h>

cpu_core_t* cpu_cores_list = NULL;
uint32_t core_count = 0;


void cpu_init(){
    cpu_cores_list = kernel_malloc(sizeof(cpu_core_t));
    cpu_cores_list->cpu_id = 0;
    cpu_cores_list->current_thread = NULL;
    cpu_cores_list->next = NULL;
    core_count = 1;

}
