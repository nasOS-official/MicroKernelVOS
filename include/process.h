#ifndef __TASKS_H
#define __TASKS_H

#include <stdint.h>

//rights
#define PROC_DRIVER_RIGHT 0b01
#define PROC_PROCLOCADER_RIGHT 0b10
#define PROC_MEM_RESERVED_RIGHT 0b100
#define PROC_MEM_ACPI_NVS_RIGHT 0b1000
#define PROC_MEM_ACPI_TABLE_RIGHT 0b10000
#define PROC_MEM_FRAMEBUFFER_RIGHT 0b100000
#define PROC_MEM_RESERVER_MAPPED_RIGHT 0b1000000

typedef enum {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_DEAD
} thread_state_t;

typedef struct process_t {
    uint32_t pid;
    uint64_t cr3;
    uint64_t rights; // driver/app; Rights for microphone, camera, screen should be resolved by ipc servers, not here. For example: Some app want microphone right -> IPC -> Microphone IPC server asks for microphone permission -> if allowed microphone ipc server will save num of allowed ipc channel and will send responces to the commands from app.
    struct task_control_block_t* first_proc_thread;
} process_t;


typedef struct task_control_block_t {
    uint32_t tid;
    uint64_t kernel_SP;
    uint64_t user_SP;
    int64_t affinity;
    thread_state_t state;
    struct process_t* process;
    struct task_control_block_t* prev_proc_thread;
    struct task_control_block_t* next_proc_thread;
} task_control_block_t;


#endif
