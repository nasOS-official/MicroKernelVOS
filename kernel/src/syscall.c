// SPDX-License-Identifier: GPL-2.0-or-later
#include "cpu.h"
#include "debug.h"
#include "mmu.h"
#include "paging.h"
#include "tasks.h"
#include <stdint.h>
#include <syscall.h>
#include <tasks.h>

syscall_ret syscall_handler(struct syscall_frame* regs){
    switch (regs->syscall_num) {
        case 30:
            return (syscall_ret){map_hardware_user(mmu_get_top_table(), get_thread_info_ptr()->process, regs->arg1, regs->arg2, regs->arg3, regs->arg4, regs->arg5), get_thread_info_ptr()};
        case 31:
            return (syscall_ret){(int64_t)alloc_user_pages(mmu_get_top_table(), regs->arg1, regs->arg2, regs->arg3), get_thread_info_ptr()};
        case 32:
            return (syscall_ret){free_user_pages(mmu_get_top_table(), regs->arg1, regs->arg2, regs->arg3), get_thread_info_ptr()};
        case 60:
            return (syscall_ret){process_create(), get_thread_info_ptr()};
        case 61:
            return (syscall_ret){process_setrights(regs->arg1, regs->arg2), get_thread_info_ptr()};
        case 62:
            return (syscall_ret){process_getrights(regs->arg1), get_thread_info_ptr()};
        case 64:
            return (syscall_ret){process_getpid(), get_thread_info_ptr()};
        case 90:
            return (syscall_ret){thread_create(regs->arg1, (uint8_t*)regs->arg2, (uint8_t*)regs->arg3, regs->arg4), get_thread_info_ptr()};
        case 92:
            return (syscall_ret){thread_gettid(), get_thread_info_ptr()};
        case 93:
            return (syscall_ret){thread_getstate(regs->arg1), get_thread_info_ptr()};
        case 94:
            return (syscall_ret){thread_setaffinity(regs->arg1, regs->arg2), get_thread_info_ptr()};
        case 95:
            return (syscall_ret){thread_getaffinity(regs->arg1), get_thread_info_ptr()};
        case 99:
            return (syscall_ret){0, sched_handler()};
        case 993:
            log_string("[APP] ");
            log_string((char*)regs->arg1); //Vulnerability: the program can pass an address from kernel space or even from any region of physical memory via the HHDM, potentially leading to the leakage of sensitive information.
            log_string("\n\r");
            return (syscall_ret){0, get_thread_info_ptr()};
        default:
            return (syscall_ret){-1, get_thread_info_ptr()};
            break;
    }
}



