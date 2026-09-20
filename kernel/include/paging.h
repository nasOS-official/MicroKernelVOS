// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __ALLOC_H
#define __ALLOC_H

#include "stdbool.h"
#include "tasks.h"
#include <stdint.h>
#include <limine.h>

#ifndef MAX_ORDER
#define MAX_ORDER 11
#endif

// ALLOC/FREE FLAGS


//Safe flag means, that you cant free or alloc pages on the already allocated or freed pages. You should know, what you are doing.
// #define SAFE_FLAG 0
#define UNSAFE_FLAG 0b01
#define READONLY_FLAG 0b10
#define NX_FLAG 0b100



// PAGE_STRUCT FLAGS

#define PAGE_FREE (1 << 0)
#define PAGE_HARDWARE (1 << 1)


// AVL flags

#define PAGE_SWAPPED (1 << 0)
#define PAGE_LAZYALLOC (1 << 1)

#define HEAP_ADDR 0xffffff0000000000



struct Page {
    uint32_t flags;
    uint8_t order;
    union {
        struct {
            struct Page* next;
            struct Page* prev;
        } free_node;


        struct {
            uint32_t counters;
            void* mapping;
        } _allocated;
    };
};

extern struct Page* buddy_alloc_ptr[MAX_ORDER];
extern struct Page* pages_map;
extern uint64_t total_pages;
extern uint64_t heap_current;

void alloc_init(struct limine_memmap_response* memmap);
struct Page* alloc_pages(uint8_t order);
void* alloc_kernel_pages(uint64_t count, uint64_t flags);
// void* alloc_ki_pages(uint64_t user_vaddr, uint64_t count, bool user, uint64_t flags);
void* alloc_user_pages(uintptr_t top_table, uint64_t user_vaddr, uint64_t count, uint64_t flags);

int64_t free_user_pages(uintptr_t top_table, uintptr_t vaddr, uint64_t count, uint64_t flags);
int64_t free_kernel_pages(uintptr_t vaddr, uint64_t count, uint64_t flags);

uintptr_t map_hardware_user(uintptr_t top_table, process_t* process, uintptr_t user_vaddr, uint64_t count, uintptr_t phys_page, uint64_t mem_type, uint64_t flags);

uintptr_t mmu_create_top_table();

uint64_t alloc_kernel_stack();

void* kernel_malloc(uint64_t size);
void kernel_free(void* addr);



#endif
