// SPDX-License-Identifier: GPL-2.0-or-later

#include "riscv64_mmu.h"
#include "../csr.h"
#include "mmu.h"
#include "debug.h"
#include <stdbool.h>
#include <stdint.h>

const mmu_level_desc_t mmu_levels_desc[4] = {
    { // VPN0
        .shift = 12,
        .mask = 0x1FF,
        .entries_count = 512,
        .page_size = 4096,
        .can_be_huge = false,
    },
    { // VPN1
        .shift = 21,
        .mask = 0x1FF,
        .entries_count = 512,
        .page_size = 4096 * 512,
        .can_be_huge = true,
    },
    { // VPN2
        .shift = 30,
        .mask = 0x1FF,
        .entries_count = 512,
        .page_size = 4096 * 512 * 512,
        .can_be_huge = true,
    },
    { // VPN3
        .shift = 39,
        .mask = 0x1FF,
        .entries_count = 512,
        .page_size = 0,
        .can_be_huge = true,
    },
};


mmu_layout_t mmu_layout = {
    .levels_count = 3,
    .levels = mmu_levels_desc
};

#define SATP_MODE_MASK (0xFULL << 60)

#define SATP_SV39 (8ULL << 60)
#define SATP_SV48 (9ULL << 60)

mmu_layout_t* mmu_get_layout(){
    uint64_t satp;
    __asm__ __volatile__("csrr %0, satp" : "=r"(satp));
    if (((satp & SATP_MODE_MASK) == SATP_SV48)){
        mmu_layout.levels_count = 4;
        logh("Using SV48");
    } else if ((satp & SATP_MODE_MASK) == SATP_SV39) {
        logh("Using SV39");
    }
    return &mmu_layout;
}


uint64_t mmu_get_elem_info(uintptr_t table_ptr, uintptr_t v_addr, uint8_t level, struct page_table_entry *page_table_entry){
    uint64_t num;
    num = (v_addr >> mmu_levels_desc[level].shift) & mmu_levels_desc[level].mask;
    if (num >= 512 || level >= mmu_layout.levels_count){
        return -1;
    }
    rv64_mmu_entry *table = (void*)table_ptr;

    page_table_entry->address = (table[num].valid ? table[num].address : 0);
    page_table_entry->present = table[num].valid; 
    page_table_entry->writable = table[num].writable; 
    page_table_entry->accessed = table[num].accessed;
    page_table_entry->user = table[num].user;
    page_table_entry->dirty = table[num].dirty; 
    page_table_entry->global = table[num].global;  
    page_table_entry->hardware = table[num].rsw & 1;
    page_table_entry->no_execute = !table[num].xecute;
    page_table_entry->ishugepage = (table[num].writable || table[num].xecute || table[num].readable) && (level != 0); 
    page_table_entry->writethrough = 0;
    page_table_entry->nocache = 0;
    page_table_entry->paged = (table[num].valid ? 0 : table[num].address & 1);
    page_table_entry->lazyalloc = (table[num].valid ? 0 : (table[num].address >> 1) & 1);
    page_table_entry->ispage = (table[num].writable || table[num].xecute || table[num].readable); 
    page_table_entry->size = (page_table_entry->ispage ? mmu_levels_desc[level].page_size : 0);


    return 0;
}
uint64_t mmu_set_elem_info(uintptr_t table_ptr, uintptr_t v_addr, uint8_t level, struct page_table_entry *page_table_entry){
    uint64_t num = (v_addr >> mmu_levels_desc[level].shift) & mmu_levels_desc[level].mask;
    if (num >= 512 || level >= mmu_layout.levels_count){
        return -1;
    }
    rv64_mmu_entry *table = (void*)table_ptr;

    bool is_leaf = (page_table_entry->ishugepage || level == 0) && page_table_entry->present;

    table[num].address = (page_table_entry->present ? page_table_entry->address : (page_table_entry->paged ? 1 : 0) | (page_table_entry->lazyalloc ? 2 : 0));
    table[num].valid = page_table_entry->present; 
    
    table[num].readable = is_leaf & 1;
    table[num].writable = is_leaf & page_table_entry->writable; 
    table[num].xecute = is_leaf & !page_table_entry->no_execute; 
    
    table[num].user = is_leaf ? page_table_entry->user : 0; 
    table[num].accessed = is_leaf ? 1 : 0; 
    table[num].dirty = (is_leaf && page_table_entry->writable) ? 1 : 0;

    table[num].global = page_table_entry->global; 
    table[num].rsw = (page_table_entry->hardware ? 1 : 0); 

    return 0;
}

uintptr_t mmu_get_top_table(){
    uint64_t satp;
    __asm__ __volatile__("csrr %0, satp" : "=r"(satp));
    return (satp & SATP64_PPN_MASK) << 12;
}


void mmu_set_top_table(uintptr_t addr) {
    uint64_t satp;
    
    __asm__ __volatile__("csrr %0, satp" : "=r"(satp));
    
    satp = (satp & ~SATP64_PPN_MASK) | ((addr >> 12) & SATP64_PPN_MASK);
    
    __asm__ __volatile__(
        "csrw satp, %0\n\t"
        "sfence.vma"
        :
        : "r"(satp)
        : "memory"
    );
}

void mmu_update_addr(uintptr_t addr){
    __asm__ __volatile__("sfence.vma %0, zero" :: "r"(addr) : "memory");
}

void mmu_update_full(){
    __asm__ __volatile__("sfence.vma zero, zero" ::: "memory");
}