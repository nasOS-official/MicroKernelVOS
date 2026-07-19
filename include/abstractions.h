// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __ABSTRACTIONS_H
#define __ABSTRACTIONS_H
#include <stdint.h>
#include <tasks.h>

void set_thread_info_ptr(task_control_block_t* ptr);
task_control_block_t* get_thread_info_ptr();
#endif