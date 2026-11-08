// SPDX-License-Identifier: GPL-2.0-or-later
#include "cpu.h"
#include "cpui.h"
#include "limine_requests.h"
#include "mmu.h"
#include "paging.h"
#include "panic.h"
#include "stdbool.h"
#include "stddef.h"
#include <stdint.h>
#include <snprintf.h>
#include <string.h>
#include <tasks.h>
#include <cpu.h>
#include "debug.h"

void insert_ready(cpu_core_t* cpu, task_control_block_t* t);

uint32_t process_counter = 1;

uint64_t process_bitmap[65536 / sizeof(uint64_t) / 8];

process_t** process_table_l1[256];

void process_setptr(uint16_t pid, process_t* ptr){
    uint8_t pid_l1 = pid >> 8;
    if (process_table_l1[pid_l1] == NULL && ptr != NULL){
        process_table_l1[pid_l1] = kernel_zalloc(256 * sizeof(uintptr_t));
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
        proc->threads_table_l1[tid_l1] = kernel_zalloc(64 * sizeof(uintptr_t));
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
    process_t* addr = kernel_zalloc(sizeof(process_t));

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

    task_control_block_t* addr = kernel_zalloc(sizeof(task_control_block_t));
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

    cur_proc->thread_counter++;

    addr->tid = ((uint32_t)pid << 16) | ltid;
    addr->process = cur_proc;
    addr->affinity = -1;
    addr->kernel_SP = alloc_kernel_stack();
    addr->initial_kernel_SP = addr->kernel_SP;
    addr->user_SP = (uint64_t)stack_pointer;
    addr->user_IP = (uint64_t)entry_point;
    addr->state = THREAD_READY;
    addr->wait_for_tick = 0;
    addr->base_priority = priority;
    addr->current_priority = priority;
    addr->inherited_priority = 0;
    addr->quantum_remaining = QUANTUM_TICKS;
    
    thread_setptr(addr->tid, addr);

    prepare_stack(addr);
    

    task_control_block_t* current = get_thread_info_ptr();
    int target_cpu = (current->affinity == -1) ? current->cpu_id : current->affinity;

    addr->cpu_id = target_cpu;

    insert_ready(cpu_list[target_cpu], addr);




    return addr->tid;
}

task_control_block_t* thread_idle_create(uint32_t cpu_id){

    task_control_block_t* idle = kernel_zalloc(sizeof(task_control_block_t));



    process_bitmap[0] |= 1;
    idle->tid = 0; 
    
    idle->process = NULL; 
    
    idle->cpu_id = cpu_id; 
    idle->affinity = cpu_id;
    
    idle->kernel_SP = alloc_kernel_stack();
    idle->initial_kernel_SP = idle->kernel_SP;
    idle->user_SP = 0;
    idle->user_IP = 0;
    idle->state = THREAD_IDLE;
    idle->base_priority = 0;
    idle->current_priority = 0;
    idle->inherited_priority = 0;
    idle->quantum_remaining = QUANTUM_TICKS;

    cpu_list[cpu_id]->current_thread = idle;
    return idle;
    //after this use set_thread_info_ptr on every core to set idle thread as current
}

intptr_t thread_remove(uint32_t tid){
    // блокируем работу всех планировщиков, мутекс короче делаем, хотя пока однопоток, можно не делать....
    if (tid == 0){
        return -1;
    }
    task_control_block_t* thread = thread_getptr(tid);
    task_control_block_t* current_thread = get_thread_info_ptr();
    if (current_thread != NULL && thread->process->pid != current_thread->process->pid){
        return -1;
    }
    thread->process->threads_bitmap[tid & 0xFFFF / sizeof(uint64_t)] &= ~(1ULL << (tid % (sizeof(uint64_t) * 8)));
    thread->process->thread_counter--;
    thread_setptr(tid, NULL);
    free_kernel_stack(thread->initial_kernel_SP);
    //удалить ядерный стек
    //удалить из планировщика, еще проверить, если поток текущий, значит нужно переключить на другой поток и затем уже удалять, либо брать и помечать поток завершенным, убирать из списка готовых, после чего удалять в какой-то момент поток.
    //сделать очередь потоков на удаление!!!!! Делаем блокировку этой хрени в момент запуска планировщика, чтобы в нее не полезли другие планировщики, после чего удаляем, если другой планировщик видит, что щас занята эта штука, то просто переходит к выбору задачи. 
    //удалить поток из очередей планировщика, заблокировать работу планировщиков спинлоком
    kernel_free(thread);
    // если потоков 0, то удалить процесс
    return 0;
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


task_control_block_t* pick_next_thread(cpu_core_t* cpu) {
    if (cpu->ready_summary == 0) {
        return NULL;
    }
    

    int highestPrio = 31 - __builtin_clz(cpu->ready_summary);
    
    task_control_block_t* t = cpu->ready_head[highestPrio];
    
    cpu->ready_head[highestPrio] = t->next;
    if (cpu->ready_head[highestPrio] == NULL) {
        cpu->ready_tail[highestPrio] = NULL;
        cpu->ready_summary &= ~(1 << highestPrio);
    }
    
    cpu->ready_count--;
    return t;
}

void insert_ready(cpu_core_t* cpu, task_control_block_t* t) {
    int prio = t->current_priority;
    cpu->ready_summary |= (1 << prio);
    t->next = NULL;
    if (cpu->ready_tail[prio] == NULL) {
        cpu->ready_head[prio] = t;
        cpu->ready_tail[prio] = t;
    } else {
        cpu->ready_tail[prio]->next = t;
        cpu->ready_tail[prio] = t;
    }
    t->state = THREAD_READY;
    cpu->ready_count++;
}



task_control_block_t* try_steal_work(cpu_core_t* cpu) {
    uint32_t my_id = cpu->cpu_id;
    
    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        if (cpu_list[i] == NULL) continue;
        if (i == my_id) continue;
        
        cpu_core_t* victim = cpu_list[i];
        
        if (victim->ready_count <= 1) continue;
        if (victim->ready_summary == 0) continue;

        int prio = 31 - __builtin_clz(victim->ready_summary);

        task_control_block_t* stolen = victim->ready_head[prio];
        task_control_block_t* prev = NULL;

        while (stolen != NULL) {
            if (stolen->affinity == -1) {
                
                if (prev == NULL) {
                    victim->ready_head[prio] = stolen->next;
                } else {
                    prev->next = stolen->next;
                }
                
                if (victim->ready_head[prio] == NULL) {
                    victim->ready_tail[prio] = NULL;
                    victim->ready_summary &= ~(1 << prio);
                }
                
                victim->ready_count--;

                return stolen;
            }
            

            prev = stolen;
            stolen = stolen->next;
        }
    }
    
    return NULL;
}

task_control_block_t* schedule() {
    task_control_block_t* current = get_thread_info_ptr();
    int cpu_id = current->cpu_id;

    if (current->state == THREAD_RUNNING && current->process != NULL) {
        current->state = THREAD_READY;
        insert_ready(cpu_list[cpu_id], current);
    }
    
    task_control_block_t* next = pick_next_thread(cpu_list[cpu_id]);
    
    while (next == NULL || next->state == THREAD_IDLE) {
        task_control_block_t* stolen = try_steal_work(cpu_list[cpu_id]);

        if (stolen != NULL) {
            next = stolen;
            break;
        }
    
        next = pick_next_thread(cpu_list[cpu_id]);
    }
    
    if (next != current) {
        next->cpu_id = cpu_id;
        cpu_list[cpu_id]->current_thread = next;
    }
    next->state = THREAD_RUNNING;
    
    return next;
}

void start_sched(){
    task_control_block_t* current = get_thread_info_ptr();
    
    current->quantum_remaining--;
    
    if (current->quantum_remaining <= 0) {
        if (current->current_priority > current->base_priority &&
            current->inherited_priority == 0) {
            current->current_priority--;
        }
        current->quantum_remaining = QUANTUM_TICKS;
    }

    
    task_control_block_t* task = schedule();
    
    set_thread_info_ptr(task);
    logk("SET TABLE");
    mmu_set_top_table(task->process->cr3);
    logk("OK!");
    start_timer(10000);
    userspace_jmp(task);
}





task_control_block_t* sched_handler(){
    task_control_block_t* current = get_thread_info_ptr();
    
    current->quantum_remaining--;
    
    if (current->quantum_remaining <= 0) {
        if (current->current_priority > current->base_priority &&
            current->inherited_priority == 0) {
            current->current_priority--;
        }
        current->quantum_remaining = QUANTUM_TICKS;
    }
    
    task_control_block_t* next = schedule();


    logk("HELLO SCHED!");

    set_thread_info_ptr(next);
    if (current->process != next->process) {
        mmu_set_top_table(next->process->cr3);
    }
    start_timer(10000);
    return next;
}