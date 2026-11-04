// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __TASKS_H
#define __TASKS_H

#include <stdint.h>
#include "cpu.h"

#define MAX_PRIORITY 32
#define MAX_THREADS_PER_PROC 4096
#define QUANTUM_TICKS 15

//rights
#define PROC_DRIVER_RIGHT 0b01
#define PROC_PROCLOCADER_RIGHT 0b10
#define PROC_MEM_RESERVED_RIGHT 0b100
#define PROC_MEM_ACPI_NVS_RIGHT 0b1000
#define PROC_MEM_ACPI_TABLE_RIGHT 0b10000
#define PROC_MEM_FRAMEBUFFER_RIGHT 0b100000
#define PROC_ERR_RIGHT (1ULL << 63)

typedef enum {
    THREAD_NOT_EXISTS = 0,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED_IPC,
    THREAD_BLOCKED_MUTEX,
    THREAD_DEAD,
    THREAD_IDLE
} thread_state_t;


typedef struct process_t {
    uint16_t pid;
    uint64_t cr3;
    uint64_t rights; // driver/app; Rights for microphone, camera, screen should be resolved by ipc servers, not here. For example: Some app want microphone right -> IPC -> Microphone IPC server asks for microphone permission -> if allowed microphone ipc server will save num of allowed ipc channel and will send responces to the commands from app.
    uint64_t resident_counter; // resident memory in pages
    uint64_t mmio_counter; // driver memory in pages
    uint64_t shared_counter; // shared memory in pages
    struct task_control_block_t** threads_table_l1[64];
    uint64_t threads_bitmap[MAX_THREADS_PER_PROC / sizeof(uint64_t) / 8];
    struct hw_data_t hw_data;
} process_t;


typedef struct task_control_block_t {
    uint32_t tid; // pid << 16 | num
    uint32_t cpu_id;
    uint64_t kernel_SP;
    uint64_t user_SP;
    uint64_t user_IP;
    int64_t affinity;
    thread_state_t state;
    uint64_t wait_for_tick; // 0 to disable wait
    uint8_t base_priority;
    uint8_t current_priority;
    uint8_t inherited_priority;
    uint32_t lastCpu;
    uint32_t quantum_remaining;
    struct process_t* process;
    struct task_control_block_t* prev;
    struct task_control_block_t* next;
} task_control_block_t;




void start_sched();
task_control_block_t* sched_handler();

process_t* process_getptr(uint16_t pid);

int32_t process_create(); // -1 = err; returns 32 bit value
uint64_t thread_create(uint16_t pid, uint8_t* entry_point, uint8_t* stack_pointer, uint8_t priority);
uint64_t process_getrights(uint16_t pid);
int64_t process_setrights(uint16_t pid, uint64_t rights);
int64_t process_ki_setrights(uint16_t pid, uint64_t rights);
uint32_t thread_gettid();
uint16_t process_getpid();
int64_t thread_setaffinity(uint32_t tid, int64_t affinity);
int64_t thread_getaffinity(uint32_t tid);
thread_state_t thread_getstate(uint32_t tid);
task_control_block_t* thread_idle_create(uint32_t cpu_id);

#endif
