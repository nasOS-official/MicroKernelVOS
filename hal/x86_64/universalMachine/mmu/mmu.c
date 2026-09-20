// SPDX-License-Identifier: GPL-2.0-or-later

#include "x86_64_mmu.h"
#include "mmu.h"
#include <stdbool.h>
#include <stdint.h>

const mmu_level_desc_t mmu_levels_desc[4] = {
    { //PT
        .shift = 12,
        .mask = 0x1FF,
        .entries_count = 512,
        .page_size = 4096,
        .can_be_huge = false,
    },
    { //PD
        .shift = 21,
        .mask = 0x1FF,
        .entries_count = 512,
        .page_size = 4096 * 512,
        .can_be_huge = true,
    },
    { // PDPT
        .shift = 30,
        .mask = 0x1FF,
        .entries_count = 512,
        .page_size = 4096 * 512 * 512,
        .can_be_huge = true,
    },
    { // PML4
        .shift = 39,
        .mask = 0x1FF,
        .entries_count = 512,
        .page_size = 0,
        .can_be_huge = false,
    },
};


mmu_layout_t mmu_layout = {
    .levels_count = 4,
    .levels = mmu_levels_desc
};


mmu_layout_t* mmu_get_layout(){
    return &mmu_layout;
}


uint64_t mmu_get_elem_info(uintptr_t table_ptr, uintptr_t v_addr, uint8_t level, struct page_table_entry *page_table_entry){
    uint64_t num;
    num = (v_addr >> mmu_levels_desc[level].shift) & mmu_levels_desc[level].mask;
    if (num >= 512 || level >= 4){
        return -1;
    }
    x86_64_page_table_entry *table = (void*)table_ptr;

    page_table_entry->address = table[num].address;
    page_table_entry->present = table[num].present; 
    page_table_entry->writable = table[num].writable; 
    page_table_entry->accessed = table[num].accessed;
    page_table_entry->user = table[num].user; 
    page_table_entry->dirty = table[num].dirty; 
    page_table_entry->global = table[num].global; 
    page_table_entry->paged = table[num].available1 & 1; 
    page_table_entry->lazyalloc = (table[num].available1 >> 1) & 1; 
    page_table_entry->hardware = (table[num].available1 >> 2) & 1; 
    page_table_entry->no_execute = table[num].no_execute; 
    page_table_entry->ispage = ((level == 3 || (level != 3 && level != 0 && table[num].pat_ps)) && table[num].present ? 1 : 0); 
    page_table_entry->ishugepage = ((level != 3 && level != 0 && table[num].pat_ps) && table[num].present ? 1 : 0); 
    page_table_entry->nocache = table[num].pcd;
    page_table_entry->writethrough = table[num].pwt;
    page_table_entry->size = (page_table_entry->ispage ? mmu_levels_desc[level].page_size : 0);
    return 0;
}


uint64_t mmu_set_elem_info(uintptr_t table_ptr, uintptr_t v_addr, uint8_t level, struct page_table_entry *page_table_entry){
    uint64_t num;
    num = (v_addr >> mmu_levels_desc[level].shift) & mmu_levels_desc[level].mask;
    if (num >= 512 || level >= 4){
        return -1;
    }
    x86_64_page_table_entry *table = (void*)table_ptr;

    table[num].address = page_table_entry->address;
    table[num].present = page_table_entry->present; 
    table[num].writable = page_table_entry->writable; 
    table[num].user = page_table_entry->user; 
    table[num].dirty = page_table_entry->dirty; 
    table[num].global = page_table_entry->global; 
    table[num].available1 = (page_table_entry->paged ? 1 : 0) | (page_table_entry->lazyalloc ? 2 : 0) | (page_table_entry->hardware ? 4 : 0); 
    table[num].no_execute = page_table_entry->no_execute; 
    table[num].pat_ps = page_table_entry->ishugepage; 
    table[num].pcd = page_table_entry->nocache;
    table[num].pwt = page_table_entry->writethrough;



    return 0;
}

uintptr_t mmu_get_top_table(){
    uintptr_t pml4_physical;
    __asm__ volatile("mov %%cr3, %0" : "=r"(pml4_physical));
    return pml4_physical;
}

void mmu_set_top_table(uintptr_t addr){
    __asm__ volatile (
        "movq %0, %%cr3"
        :
        : "r" (addr)
        : "memory"
    );

}

void mmu_update_addr(uintptr_t addr){
    __asm__ volatile("invlpg (%0)" :: "r"(addr) : "memory");
}

void mmu_update_full(){
    
}