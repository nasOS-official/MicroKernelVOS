// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __ALLOC_H
#define __ALLOC_H

#include "stdbool.h"
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


// PAGE_STRUCT FLAGS

#define PAGE_FREE (1 << 0)
#define PAGE_HARDWARE (1 << 1)


// AVL flags

#define PAGE_SWAPPED (1 << 0)
#define PAGE_LAZYALLOC (1 << 1)

#define HEAP_ADDR 0xffffff0000000000

typedef struct page_table_entry {
    uint64_t present    : 1;
    uint64_t writable   : 1;
    uint64_t user       : 1;
    uint64_t pwt        : 1;
    uint64_t pcd        : 1;
    uint64_t accessed   : 1;
    uint64_t dirty      : 1;
    uint64_t pat        : 1;
    uint64_t global     : 1;
    uint64_t available1 : 3;
    uint64_t address    : 40;
    uint64_t available2 : 11;
    uint64_t no_execute : 1;
} page_table_entry;

typedef struct page_tree {
    uint8_t min_level_exists;
    page_table_entry pml4;
    page_table_entry pdpt;
    page_table_entry pd;
    page_table_entry pt;
} page_tree;


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
void* alloc_ki_pages(uint64_t user_vaddr, uint64_t count, bool user, uint64_t flags);
void* alloc_user_pages(uint64_t user_vaddr, uint64_t count, uint64_t flags);

void page_set(uint64_t pml4_phys_ptr, uint64_t virt_ptr, int level, page_table_entry value);
page_tree page_tree_get(uint64_t pml4_phys_ptr, uint64_t virt_ptr);

int64_t free_user_pages(uint64_t user_vaddr, uint64_t count, uint64_t flags);
int64_t free_kernel_pages(uint64_t user_vaddr, uint64_t count, uint64_t flags);

uint64_t map_hardware_user(uint64_t user_vaddr,	uint64_t count, uint64_t pfn, uint64_t mem_type, uint64_t flags);


uint64_t alloc_kernel_stack();

void* kernel_malloc(uint64_t size);
void kernel_free(void* addr);



#endif
