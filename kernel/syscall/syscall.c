// SPDX-License-Identifier: GPL-2.0-or-later
#include "cpu.h"
#include "debug.h"
#include "mmu.h"
#include "paging.h"
#include "tasks.h"
#include <stdint.h>
#include <syscall.h>

int64_t syscall_handler(struct syscall_frame* regs){
    switch (regs->syscall_num) {
        case 30:
            return map_hardware_user(mmu_get_top_table(), get_thread_info_ptr()->process, regs->arg1, regs->arg2, regs->arg3, regs->arg4, regs->arg5);
        case 31:
            return (int64_t)alloc_user_pages(mmu_get_top_table(), regs->arg1, regs->arg2, regs->arg3);
        case 32:
            return free_user_pages(mmu_get_top_table(), regs->arg1, regs->arg2, regs->arg3);
        case 60:
            return process_create();
        case 61:
            return process_setrights(regs->arg1, regs->arg2);
        case 62:
            return process_getrights(regs->arg1);
        case 64:
            return process_getpid();
        case 90:
            return thread_create(regs->arg1, (uint8_t*)regs->arg2, (uint8_t*)regs->arg3, regs->arg4);
        case 92:
            return thread_gettid();
        case 93:
            return thread_getstate(regs->arg1);
        case 94:
            return thread_setaffinity(regs->arg1, regs->arg2);
        case 95:
            return thread_getaffinity(regs->arg1);
        case 993:
            log_string("[APP] ");
            log_string((char*)regs->arg1); // тут беда на risc-v пока что. проверить что там по глобальным переменным.
            log_string("\n\r");
            return 0;
        default:
            return -1;
            break;
    }
}



