// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __TASKS_H
#define __TASKS_H

#include "paging.h"
#include <stdint.h>
#include <cpu.h>

//rights
#define PROC_DRIVER_RIGHT 0b01
#define PROC_PROCLOCADER_RIGHT 0b10
#define PROC_MEM_RESERVED_RIGHT 0b100
#define PROC_MEM_ACPI_NVS_RIGHT 0b1000
#define PROC_MEM_ACPI_TABLE_RIGHT 0b10000
#define PROC_MEM_FRAMEBUFFER_RIGHT 0b100000
#define PROC_MEM_RESERVER_MAPPED_RIGHT 0b1000000
#define PROC_ERR_RIGHT (1ULL << 63)

typedef enum {
    THREAD_NOT_EXISTS = 0,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_DEAD
} thread_state_t;


typedef struct process_t {
    uint32_t pid;
    uint64_t cr3;
    uint64_t rights; // driver/app; Rights for microphone, camera, screen should be resolved by ipc servers, not here. For example: Some app want microphone right -> IPC -> Microphone IPC server asks for microphone permission -> if allowed microphone ipc server will save num of allowed ipc channel and will send responces to the commands from app.
    uint64_t resident_counter; // resident memory in pages
    uint64_t mmio_counter; // driver memory in pages
    uint64_t shared_counter; // shared memory in pages
    uint32_t last_thread_num;
    struct task_control_block_t* first_proc_thread;
    struct task_control_block_t* last_proc_thread;
    struct process_t* next;
    struct process_t* prev;
} process_t;


typedef struct task_control_block_t {
    uint32_t tid; // pid << 32 | num
    uint32_t cpu_id;
    uint64_t kernel_SP;
    uint64_t user_SP;
    uint64_t user_IP;
    cpu_core_t* core;
    int64_t affinity;
    thread_state_t state;
    uint64_t wait_for_tick; // 0 to disable wait
    struct process_t* process;
    struct task_control_block_t* prev_proc_thread;
    struct task_control_block_t* next_proc_thread;
} task_control_block_t;

uint32_t process_create();
uint64_t thread_create(uint32_t pid, uint8_t* entry_point, uint8_t* stack_pointer, uint8_t priority);
uint64_t process_getrights(uint32_t pid);
int64_t process_setrights(uint32_t pid, uint64_t rights);
uint64_t thread_gettid();
uint32_t process_getpid();
int64_t thread_setaffinity(uint64_t tid, int64_t affinity);
int64_t thread_getaffinity(uint64_t tid);
thread_state_t thread_getstate(uint64_t tid);

#endif
