// SPDX-License-Identifier: GPL-2.0-or-later
#include "abstractions.h"
#include "limine_requests.h"
#include "paging.h"
#include "stdbool.h"
#include "stddef.h"
#include <math.h>
#include <stdint.h>
#include <tasks.h>
#include <cpu.h>
#include "../syscall/x86_64/msr.h"
#include "../debug/fb.h"

#define MAX_PRIORITY 32

typedef struct task_list_t{
    task_control_block_t* block;
    struct task_list_t* prev; // circular list
    struct task_list_t* next; // circular list
} task_list_t;

task_list_t* priority_list[MAX_PRIORITY];


process_t* process_first = NULL;
process_t* process_last = NULL;

uint32_t process_counter = 0;

uint64_t process_bitmap[32768 / sizeof(uint64_t) / 8];

void process_init(){
       
}

uint32_t process_create(){
    uint32_t pid = 0;
    for (uint16_t x = 0; x < 4096; x++){
        if (process_bitmap[x] != 0xFFFFFFFFFFFFFFFF){
            int bit_index = __builtin_ctzll(~process_bitmap[x]);
            pid = (x * 64) + bit_index;
            process_bitmap[x] |= (1ULL << bit_index);
        }
    }
    if (pid == 0){
        return 0;
    }

    process_t* addr = kernel_malloc(sizeof(process_t));


    if (process_first == NULL){
        process_first = addr;
    }

    addr->prev = process_last;
    addr->next = NULL;
    addr->rights = 0;
    addr->resident_counter = 0;
    addr->mmio_counter = 0;
    addr->shared_counter = 0;
    addr->pid = pid;
    addr->last_thread_num = 0;

    if (process_last != NULL){
        process_last->next = addr;
    }

    process_last = addr;
    process_counter++;
    
    return addr->pid;
}

uint64_t thread_create(uint32_t pid, uint8_t* entry_point, uint8_t* stack_pointer, uint8_t priority){
    if (priority >= MAX_PRIORITY){
        return 0;
    }

    task_control_block_t* addr = kernel_malloc(sizeof(task_control_block_t));
    
    process_t* cur_proc = process_first;
    for (uint32_t x = 0; x < process_counter; x++){
        if(cur_proc->pid == pid){
            break;
        }
        cur_proc = cur_proc->next;
    }
    if (cur_proc == NULL){
        return 0;
    }
    addr->tid = ((uint64_t)pid << 32) | ++cur_proc->last_thread_num;
    addr->process = cur_proc;
    addr->affinity = -1;
    addr->kernel_SP = alloc_kernel_stack();
    addr->user_SP = (uint64_t)stack_pointer;
    addr->user_IP = (uint64_t)entry_point;
    addr->state = THREAD_RUNNING;
    addr->wait_for_tick = 0;
    addr->prev_proc_thread = cur_proc->last_proc_thread;
    addr->next_proc_thread = NULL;
    if (cur_proc->last_proc_thread != NULL){
        cur_proc->last_proc_thread->next_proc_thread = addr;
    }
    cur_proc->last_proc_thread = addr;
    if  (cur_proc->first_proc_thread == NULL){
        cur_proc->first_proc_thread = addr;
    }

    task_list_t* task_priority_addr = kernel_malloc(sizeof(task_list_t));
    if (priority_list[priority] != NULL){
        priority_list[priority]->prev = task_priority_addr;
    }
    task_priority_addr->next = priority_list[priority];
    task_priority_addr->prev = NULL;
    priority_list[priority] = task_priority_addr;

    return addr->tid;
}

uint64_t process_getrights(uint32_t pid){
    process_t* ptr = process_first;
    if (get_thread_info_ptr()->process->pid == pid){
        return get_thread_info_ptr()->process->rights;
    }
    while (true) {
        if (ptr->pid == pid || ptr == NULL){
            break;
        }
        ptr = ptr->next;
    }
    if (ptr == NULL){
        return PROC_ERR_RIGHT;
    }

    return ptr->rights;
}

int64_t process_setrights(uint32_t pid, uint64_t rights){
    if ((get_thread_info_ptr()->process->rights & PROC_PROCLOCADER_RIGHT) == 0){
        return -1;
    }
    process_t* ptr = process_first;

    while (true) {
        if (ptr->pid == pid || ptr == NULL){
            break;
        }
        ptr = ptr->next;
    }
    if (ptr == NULL){
        return -1;
    }

    ptr->rights = rights;
    return 0;
}

uint64_t thread_gettid(){
    return get_thread_info_ptr()->tid;
}

uint32_t process_getpid(){
    return get_thread_info_ptr()->process->pid;
}

task_control_block_t* find_thread(uint64_t tid){
    process_t* current_process = process_first;
    uint32_t pid = tid >> 32;
    while (true) {
        if (current_process->pid == pid){
            break;
        }
        if (current_process->next == NULL){
            return NULL;
        }
    }

    task_control_block_t* current_thread = current_process->first_proc_thread;

    while (true) {
        if (current_thread->tid == tid){
            break;
        }
        if (current_thread->next_proc_thread == NULL){
            return NULL;
        }
    }
    return current_thread;
}

int64_t thread_setaffinity(uint64_t tid, int64_t affinity){
    task_control_block_t* current_thread = find_thread(tid);
    
    if (current_thread == NULL){
        return -2;
    }
    current_thread->affinity = affinity;
    return 0;
}


int64_t thread_getaffinity(uint64_t tid){
    task_control_block_t* current_thread = find_thread(tid);
    
    if (current_thread == NULL){
        return -2;
    }

    return current_thread->affinity;
}

thread_state_t thread_getstate(uint64_t tid){
    task_control_block_t* current_thread = find_thread(tid);
    
    if (current_thread == NULL){
        return THREAD_NOT_EXISTS;
    }
    
    return current_thread->state;
}

void start_sched(){
    set_kernel_stack((void*)priority_list[0]->block->kernel_SP);
    
}

// extern uint64_t tsc_ticks_per_ms;
// extern uint64_t read_tsc();
// extern uint64_t lapic_base;

// void sched_handler(){
//     log("HELLO SCHED!", framebuffer_request.response->framebuffers[0]);
//     uint64_t next_deadline = read_tsc() + tsc_ticks_per_ms;
//     wrmsr(IA32_TSC_DEADLINE, next_deadline);

//     *(volatile uint32_t*)(lapic_base + 0xB0) = 0;
// }