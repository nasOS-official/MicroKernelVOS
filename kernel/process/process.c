// SPDX-License-Identifier: GPL-2.0-or-later
#include "cpu.h"
#include "mmu.h"
#include "paging.h"
#include "stdbool.h"
#include "stddef.h"
#include <stdint.h>
#include <snprintf.h>
#include <string.h>
#include <tasks.h>
#include <cpu.h>
#include "debug.h"




task_control_block_t* priority_list[MAX_PRIORITY];
uint32_t priority_bitmap;



uint32_t process_counter = 0;

uint64_t process_bitmap[65536 / sizeof(uint64_t) / 8];

process_t** process_table_l1[256];

void process_setptr(uint16_t pid, process_t* ptr){
    uint8_t pid_l1 = pid >> 8;
    if (process_table_l1[pid_l1] == NULL && ptr != NULL){
        process_table_l1[pid_l1] = kernel_malloc(256 * sizeof(uintptr_t));
    } else if (process_table_l1[pid_l1] == NULL){
        return;
    }
    process_table_l1[pid_l1][pid & 0xff] = ptr;

    if (ptr == NULL){
        int c = 0;
        for (int x = 0; x < 256; x++){
            if (process_table_l1[pid_l1][x] != NULL){
                c++;
                break;
            }
        }
        if (c == 0){
            kernel_free(process_table_l1[pid_l1]);
            process_table_l1[pid_l1] = NULL;
        }
    }
}

process_t* process_getptr(uint16_t pid){
    uint8_t pid_l1 = pid >> 8;
    if (process_table_l1[pid_l1] == NULL){
        return NULL;
    }
    return process_table_l1[pid_l1][pid & 0xff];
}


void thread_setptr(uint32_t tid, task_control_block_t* ptr){
    process_t* proc = process_getptr(tid >> 16);
    uint8_t tid_l1 = (tid >> 6) & 0x3f;
    uint8_t tid_l2 = tid & 0x3f;

    if (proc->threads_table_l1[tid_l1] == NULL && ptr != NULL){
        proc->threads_table_l1[tid_l1] = kernel_malloc(64 * sizeof(uintptr_t));
    } else if (proc->threads_table_l1[tid_l1] == NULL) {
        return;
    }
    proc->threads_table_l1[tid_l1][tid_l2] = ptr;

    if (ptr == NULL){
        int c = 0;
        for (int x = 0; x < 64; x++){
            if (proc->threads_table_l1[tid_l1][x] != NULL){
                c++;
                break;
            }
        }
        if (c == 0){
            kernel_free(proc->threads_table_l1[tid_l1]);
            proc->threads_table_l1[tid_l1] = NULL;
        }
    }
}

task_control_block_t* thread_getptr(uint32_t tid){
    process_t* proc = process_getptr(tid >> 16);
    uint8_t tid_l1 = (tid >> 6) & 0x3f;
    uint8_t tid_l2 = tid & 0x3f;
    if (proc->threads_table_l1[tid_l1] == NULL){
        return NULL;
    }
    return proc->threads_table_l1[tid_l1][tid_l2];
}

int32_t process_create(){
    int32_t pid = -1;
    for (uint16_t x = 0; x < sizeof(process_bitmap) / sizeof(uint64_t); x++){
        if (process_bitmap[x] != 0xFFFFFFFFFFFFFFFF){
            int bit_index = __builtin_ctzll(~process_bitmap[x]);
            pid = (x * 64) + bit_index;
            process_bitmap[x] |= (1ULL << bit_index);
            break;
        }
    }
    if (pid == -1){
        return -1;
    }
    process_t* addr = kernel_malloc(sizeof(process_t));
    memset(addr, 0, sizeof(process_t));

    addr->cr3 = mmu_create_top_table();

    addr->pid = pid;

    process_setptr(pid, addr);

    process_counter++;
    
    return addr->pid;
}

uint64_t thread_create(uint16_t pid, uint8_t* entry_point, uint8_t* stack_pointer, uint8_t priority){
    if (priority >= MAX_PRIORITY){
        return 0;
    }

    task_control_block_t* addr = kernel_malloc(sizeof(task_control_block_t));
    process_t* cur_proc = process_getptr(pid);

    uint32_t ltid = 0;
    for (uint16_t x = 0; x < sizeof(cur_proc->threads_bitmap) / sizeof(uint64_t); x++){
        if (cur_proc->threads_bitmap[x] != 0xFFFFFFFFFFFFFFFF){
            int bit_index = __builtin_ctzll(~cur_proc->threads_bitmap[x]);
            ltid = (x * 64) + bit_index;
            cur_proc->threads_bitmap[x] |= (1ULL << bit_index);
            break;
        }
    }

    addr->tid = ((uint32_t)pid << 16) | ltid;
    addr->process = cur_proc;
    addr->affinity = -1;
    addr->kernel_SP = alloc_kernel_stack();
    addr->user_SP = (uint64_t)stack_pointer;
    addr->user_IP = (uint64_t)entry_point;
    addr->state = THREAD_RUNNING;
    addr->wait_for_tick = 0;
    thread_setptr(addr->tid, addr);

    prepare_stack(addr);
    


    // task_list_t* task_priority_addr = kernel_malloc(sizeof(task_list_t));
    // task_priority_addr->block = addr;


    // if (priority_list[priority] != NULL){
    //     priority_list[priority]->prev = task_priority_addr;
    // }
    if (priority_list[priority] == NULL){
        priority_list[priority] = addr;
        addr->task_lst.next = addr;
        addr->task_lst.prev = addr;
    } else {
        addr->task_lst.next = priority_list[priority];
        addr->task_lst.prev = priority_list[priority]->task_lst.prev;
        addr->task_lst.prev->task_lst.next = addr;
        addr->task_lst.next->task_lst.prev = addr;
    }

    return addr->tid;
}

uint64_t process_getrights(uint16_t pid){
    process_t* ptr = process_getptr(pid);
    if (ptr == NULL){
        return PROC_ERR_RIGHT;
    }

    return ptr->rights;
}

int64_t process_setrights(uint16_t pid, uint64_t rights){
    if ((get_thread_info_ptr()->process->rights & PROC_PROCLOCADER_RIGHT) == 0){
        return -1;
    }
    process_t* ptr = process_getptr(pid);

    if (ptr == NULL){
        return -1;
    }

    ptr->rights = rights;
    return 0;
}

int64_t process_ki_setrights(uint16_t pid, uint64_t rights){
    process_t* ptr = process_getptr(pid);

    if (ptr == NULL){
        return -1;
    }

    ptr->rights = rights;
    return 0;
}

uint32_t thread_gettid(){
    return get_thread_info_ptr()->tid;
}

uint16_t process_getpid(){
    return get_thread_info_ptr()->process->pid;
}



int64_t thread_setaffinity(uint32_t tid, int64_t affinity){
    task_control_block_t* current_thread = thread_getptr(tid);
    
    if (current_thread == NULL){
        return -2;
    }
    current_thread->affinity = affinity;
    return 0;
}


int64_t thread_getaffinity(uint32_t tid){
    task_control_block_t* current_thread = thread_getptr(tid);
    
    if (current_thread == NULL){
        return -2;
    }

    return current_thread->affinity;
}

thread_state_t thread_getstate(uint32_t tid){
    task_control_block_t* current_thread = thread_getptr(tid);
    
    if (current_thread == NULL){
        return THREAD_NOT_EXISTS;
    }
    
    return current_thread->state;
}

void start_sched(){
    task_control_block_t* task = NULL;
    for (int x = 0; x < MAX_PRIORITY; x++){
        if (priority_list[x] != NULL){
            priority_list[x] = priority_list[x]->task_lst.next;
            task = priority_list[x];
            break;
        }
    }
    set_thread_info_ptr(task);
    logk("SET TABLE");
    mmu_set_top_table(task->process->cr3);
    logk("OK!");
    start_timer(10000);
    userspace_jmp(task);
}

task_control_block_t* sched_handler(){
    logk("HELLO SCHED!");
    task_control_block_t* task = NULL;

    for (int x = 0; x < MAX_PRIORITY; x++){
        if (priority_list[x] != NULL){
            priority_list[x] = priority_list[x]->task_lst.next;
            task = priority_list[x];
            break;
        }
    }
    set_thread_info_ptr(task);
    mmu_set_top_table(task->process->cr3);
    start_timer(10000);
    return task;
}