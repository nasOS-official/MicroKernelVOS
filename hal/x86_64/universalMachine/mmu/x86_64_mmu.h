// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef __x86_64_MMU_H
#define __x86_64_MMU_H


#include <stdint.h>
#include <stdbool.h>

typedef struct x86_64_page_table_entry {
    uint64_t present    : 1;
    uint64_t writable   : 1;
    uint64_t user       : 1;
    uint64_t pwt        : 1;
    uint64_t pcd        : 1;
    uint64_t accessed   : 1;
    uint64_t dirty      : 1;
    uint64_t pat_ps     : 1;
    uint64_t global     : 1;
    uint64_t available1 : 3;
    uint64_t address    : 40;
    uint64_t available2 : 11;
    uint64_t no_execute : 1;
} x86_64_page_table_entry;

#endif