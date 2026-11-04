// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __SYSCALL_H
#define __SYSCALL_H
#include "cpu.h"
#include "tasks.h"
#include <stdint.h>


syscall_ret syscall_handler(struct syscall_frame* regs);
void syscall();

#endif