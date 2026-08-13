// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __SYSCALL_H
#define __SYSCALL_H
#include "cpu.h"

int64_t syscall_handler(struct syscall_frame* regs);
void syscall();

#endif