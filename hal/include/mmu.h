// SPDX-License-Identifier: GPL-2.0-or-later


#ifndef __MMU_H
#define __MMU_H


#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t  shift;
    uint64_t mask;
    uint16_t entries_count;
    uint64_t page_size;
    bool     can_be_huge;
} mmu_level_desc_t;

typedef struct {
    uint8_t               levels_count;
    const mmu_level_desc_t *levels;
} mmu_layout_t;

typedef struct page_table_entry {
    uint64_t size;
    uintptr_t address;
    uint64_t present      : 1;
    uint64_t writable     : 1;
    uint64_t user         : 1;
    uint64_t accessed     : 1;
    uint64_t dirty        : 1;
    uint64_t global       : 1;
    uint64_t paged        : 1;
    uint64_t lazyalloc    : 1;
    uint64_t hardware     : 1;
    uint64_t no_execute   : 1;
    uint64_t nocache      : 1;
    uint64_t writethrough : 1;
    uint64_t ispage       : 1;
    uint64_t ishugepage   : 1;
} page_table_entry;


mmu_layout_t* mmu_get_layout();

uint64_t mmu_get_elem_info(uintptr_t table_ptr, uint64_t num, uint8_t level, struct page_table_entry *page_table_entry);


uint64_t mmu_set_elem_info(uintptr_t table_ptr, uintptr_t v_addr, uint8_t level, struct page_table_entry *page_table_entry);

uintptr_t mmu_get_top_table();

void mmu_update_addr(uintptr_t addr);

void mmu_update();

void mmu_update_full();


#endif