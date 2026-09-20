// SPDX-License-Identifier: GPL-2.0-or-later
#include "paging.h"
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <stdint.h>
#include <limine_requests.h>
#include <snprintf.h>
#include <math.h>
#include <panic.h>
#include <tasks.h>
#include <stdlib.h>
#include "cpu.h"
#include "mmu.h"


struct Page* buddy_alloc_ptr[MAX_ORDER];
struct Page* pages_map = NULL;
uint64_t total_pages = 0;
uint64_t heap_current = HEAP_ADDR;
uint64_t allocated_pages = 0;

static mmu_layout_t* mmu_layout;

static void list_add(struct Page** head, struct Page* node) {
    node->free_node.next = *head;
    node->free_node.prev = NULL;
    if (*head) (*head)->free_node.prev = node;
    *head = node;
}

static void list_remove(struct Page** head, struct Page* node) {
    if (node->free_node.prev) node->free_node.prev->free_node.next = node->free_node.next;
    else *head = node->free_node.next;
    if (node->free_node.next) node->free_node.next->free_node.prev = node->free_node.prev;
}

static void clear_page_table(void* virt_addr) {
    uint64_t* ptr = (uint64_t*)virt_addr;
    for (int i = 0; i < 512; i++) {
        ptr[i] = 0;
    }
}

void alloc_init(struct limine_memmap_response* memmap) {
    // uint64_t pml4_physical = mmu_get_top_table();
    mmu_layout = mmu_get_layout();

    uint64_t hhdm_offset = hhdm_request.response->offset;

    uint64_t mem_size = memmap->entries[memmap->entry_count - 1]->base + memmap->entries[memmap->entry_count - 1]->length;
    total_pages = mem_size / mmu_layout->levels[0].page_size;
    uint64_t pages_map_size = total_pages * sizeof(struct Page);

    uint64_t base_addr = 0;
    for (uint64_t x = 0; x < memmap->entry_count; x++) {
        if (memmap->entries[x]->length >= pages_map_size && memmap->entries[x]->type == LIMINE_MEMMAP_USABLE) {
            base_addr = memmap->entries[x]->base;
            break;
        }
    }

    pages_map = (struct Page*)(hhdm_offset + base_addr);
    uint64_t start_sel = 0;
    for (uint64_t i = 0; i < total_pages; i++) {
        pages_map[i].flags = 0;
        for (uint64_t x = start_sel; x < memmap->entry_count; x++){
            if (memmap->entries[x]->base >> mmu_layout->levels[0].shift <= i && (memmap->entries[x]->base + memmap->entries[x]->length) >> mmu_layout->levels[0].shift > i){
                start_sel = x;
                    switch (memmap->entries[x]->type) {
                        case LIMINE_MEMMAP_RESERVED:
                        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
                        case LIMINE_MEMMAP_ACPI_NVS:
                        case LIMINE_MEMMAP_FRAMEBUFFER:
                        case LIMINE_MEMMAP_RESERVED_MAPPED:
                            pages_map[i].flags |= PAGE_HARDWARE;
                            break;
                        default:
                            break;
                    }

                break;
            }
        }
        pages_map[i].order = 0;
        pages_map[i].free_node.next = NULL;
        pages_map[i].free_node.prev = NULL;
    }

    uint64_t metadata_start_pfn = base_addr /  mmu_layout->levels[0].page_size;
    uint64_t metadata_end_pfn = (base_addr + pages_map_size + mmu_layout->levels[0].page_size - 1) /  mmu_layout->levels[0].page_size;
    uint64_t free_pages = 0;
    for (uint64_t x = 0; x < memmap->entry_count; x++) {
        if (memmap->entries[x]->type != LIMINE_MEMMAP_USABLE) continue;

        uint64_t start_pfn = memmap->entries[x]->base / mmu_layout->levels[0].page_size;
        uint64_t end_pfn = (memmap->entries[x]->base + memmap->entries[x]->length) / mmu_layout->levels[0].page_size;

        for (uint64_t pfn = start_pfn; pfn < end_pfn; ) {
            if (pfn >= metadata_start_pfn && pfn < metadata_end_pfn) {
                pfn = metadata_end_pfn;
                continue;
            }

            uint8_t order = 0;
            while (order < MAX_ORDER - 1 &&
                   (pfn % (1 << (order + 1))) == 0 &&
                   (pfn + (1 << (order + 1))) <= end_pfn) {

                if (pfn < metadata_start_pfn && (pfn + (1 << (order + 1))) > metadata_start_pfn) {
                    break;
                }
                order++;
            }

            struct Page* page = &pages_map[pfn];
            page->flags |= PAGE_FREE;
            page->order = order;
            list_add(&buddy_alloc_ptr[order], page);

            pfn += (1 << order);
            free_pages += (1 << order);
        }
    }
    allocated_pages = total_pages - free_pages;
}

struct Page* alloc_pages(uint8_t order) {
    for (uint8_t i = order; i < MAX_ORDER; i++) {
        if (buddy_alloc_ptr[i] != NULL) {
            struct Page* page = buddy_alloc_ptr[i];
            list_remove(&buddy_alloc_ptr[i], page);
            page->flags &= ~PAGE_FREE;

            while (i > order) {
                i--;
                uint64_t pfn = page - pages_map;
                uint64_t buddy_pfn = pfn + (1 << i);
                struct Page* buddy = &pages_map[buddy_pfn];

                buddy->order = i;
                buddy->flags |= PAGE_FREE;
                list_add(&buddy_alloc_ptr[i], buddy);
            }
            allocated_pages += (1 << order);
            page->order = order;
            return page;
        }
    }
    return NULL;
}

void free_buddy_pages(struct Page* page, uint8_t order) {
    uint64_t pfn = page - pages_map;

    for (uint8_t i = order; i < MAX_ORDER - 1; i++) {
        uint64_t buddy_pfn = pfn ^ (1 << i);
        if (buddy_pfn >= total_pages) break;

        struct Page* buddy = &pages_map[buddy_pfn];

        if (!(buddy->flags & PAGE_FREE) || buddy->order != i) {
            break;
        }

        list_remove(&buddy_alloc_ptr[i], buddy);
        buddy->flags &= ~PAGE_FREE;

        if (buddy_pfn < pfn) {
            pfn = buddy_pfn;
        }
    }

    allocated_pages -= (1 << order);
    struct Page* final_page = &pages_map[pfn];
    final_page->order = order;
    final_page->flags |= PAGE_FREE;
    list_add(&buddy_alloc_ptr[order], final_page);
}





bool scan_ki_pages(uintptr_t table_ptr, uint8_t level, uintptr_t curr_addr, uintptr_t max_addr, bool searchtoalloc){
    page_table_entry table_entry;
    uint64_t span = 1ull << mmu_layout->levels[level].shift;
    while (curr_addr <= max_addr) {
        mmu_get_elem_info(table_ptr, curr_addr, level, &table_entry);
        if (!table_entry.present){
            if (!searchtoalloc){
                return false;
            }
            curr_addr += span;
            continue;
            
        }
        if (level == 0){
            if (searchtoalloc){
                return false;
            } 
            curr_addr += span;
            continue;
        }
        
        uint64_t smax_addr = MIN(max_addr, curr_addr | (span - 1));
        if (!scan_ki_pages((table_entry.address << mmu_layout->levels[0].shift) + hhdm_request.response->offset, level - 1, curr_addr, smax_addr, searchtoalloc)){
            return false;
        }

        curr_addr = smax_addr + 1;
    }
    return true;
}

bool scan_pages(uintptr_t top_table, uint64_t vaddr, uint64_t count, bool searchtoalloc){
    uint64_t max_addr = vaddr + ((count - 1) * 0x1000);
    return scan_ki_pages(hhdm_request.response->offset + top_table, mmu_layout->levels_count - 1, vaddr, max_addr, searchtoalloc);
}



int64_t free_ki_pages(uintptr_t table_ptr, uint8_t level, uintptr_t curr_addr, uintptr_t max_addr){
    page_table_entry table_entry;
    uintptr_t span = 1ull << mmu_layout->levels[level].shift;
    while (curr_addr <= max_addr) {
        mmu_get_elem_info(table_ptr, curr_addr, level, &table_entry);
        if (!table_entry.present){ // page or level doesn't exist
            curr_addr += span;
            continue;
        }
        if (level == 0){ // next level doesn't exist; page exists
            uintptr_t page = table_entry.address;                      
            if (pages_map[page].flags & PAGE_FREE){
                panic();
            }
            //todo: remove free mem tables if entire region is free
            table_entry.present = 0;
            table_entry.address = 0;
            mmu_set_elem_info(table_ptr, curr_addr, level, &table_entry);
            mmu_update_addr(curr_addr);


            if (!(pages_map[page].flags & PAGE_HARDWARE)){
                pages_map[page]._allocated.counters--;

                if (pages_map[page]._allocated.counters == 0){
                    free_buddy_pages(&pages_map[page], 0);
                }
            }

            curr_addr += span;
            continue;
        }
        // next level exists
        uintptr_t smax_addr = MIN(max_addr, curr_addr | (span - 1));
        if (free_ki_pages((table_entry.address << mmu_layout->levels[0].shift) + hhdm_request.response->offset, level - 1, curr_addr, smax_addr) != 0){
            return -1;
        }

        curr_addr = smax_addr + 1;
    }
    return 0;
}

//where is vaddr requires ptr pfn << mmu_layout->levels[0].shift

int64_t free_user_pages(uintptr_t top_table, uintptr_t vaddr, uint64_t count, uint64_t flags){
    if (count == 0 || allocated_pages < count || (vaddr + (count * mmu_layout->levels[0].page_size) >= 0x0000800000000000)){
        return -1;
    }

    if (!(flags & UNSAFE_FLAG)){
        if (!scan_pages(top_table, vaddr, count, false)){
            return -1;
        }
    }
    
    uintptr_t max_addr = vaddr + ((count - 1) * 0x1000);

    return free_ki_pages(hhdm_request.response->offset + top_table, 
                   mmu_layout->levels_count - 1,
                   vaddr,
                   max_addr);
    // return free_ki_pages(user_vaddr, count, 1, flags);
}


int64_t free_kernel_pages(uintptr_t vaddr, uint64_t count, uint64_t flags){
    if (count == 0 || allocated_pages < count || vaddr < 0x0000800000000000){
        return -1;
    }

    if ((!(flags & UNSAFE_FLAG)) && (!scan_pages(mmu_get_top_table(), vaddr, count, false))){
        return -1;
    }
    // return free_ki_pages(user_vaddr, count, 0, flags);
    uint64_t max_addr = vaddr + ((count - 1) * mmu_layout->levels[0].page_size);
    
    return free_ki_pages(hhdm_request.response->offset + mmu_get_top_table(), 
                   mmu_layout->levels_count - 1,
                   vaddr,
                   max_addr);
}



bool alloc_ki_pages(uintptr_t table_ptr, uint8_t level, uintptr_t curr_addr, uintptr_t max_addr, bool user, bool nx){
    page_table_entry table_entry;
    uint64_t span = 1ull << mmu_layout->levels[level].shift;
    while (curr_addr <= max_addr) {
        mmu_get_elem_info(table_ptr, curr_addr, level, &table_entry);
        if (!table_entry.present){
            if (level == 0){
                //map page
                struct Page* phys_page = alloc_pages(0);
                if (!phys_page) panic();

                uintptr_t paddr = (phys_page - pages_map);
                table_entry.address = paddr;
                table_entry.user = user;
                table_entry.writable = 1;
                table_entry.present = 1;
                table_entry.no_execute = nx;
                mmu_set_elem_info(table_ptr, curr_addr, level, &table_entry);
                mmu_update_addr(curr_addr);
                curr_addr += span;
                continue;
            } else {
                //add next level
                struct Page* new_table_page = alloc_pages(0);
                if (!new_table_page) panic();

                uint64_t new_table_phys = (new_table_page - pages_map) << mmu_layout->levels[0].shift;
                clear_page_table((void*)(hhdm_request.response->offset + new_table_phys));

                table_entry.address = new_table_phys >> mmu_layout->levels[0].shift;
                table_entry.user = user;
                table_entry.writable = 1;
                table_entry.present = 1;
                mmu_set_elem_info(table_ptr, curr_addr, level, &table_entry);
            }
        }

        if (level == 0){
            curr_addr += span;
            continue;
        } 

        
        uint64_t smax_addr = MIN(max_addr, curr_addr | (span - 1));
        if (!alloc_ki_pages((table_entry.address << mmu_layout->levels[0].shift) + hhdm_request.response->offset, level - 1, curr_addr, smax_addr, user, nx)){
            return false;
        }

        curr_addr = smax_addr + 1;
    }
    return true;
}



void* alloc_kernel_pages(uint64_t count, uint64_t flags){
    // void* ptr = alloc_ki_pages(heap_current, count, false, flags);
    // if (ptr == NULL){
    //     return NULL;
    // }

    if ((!(flags & UNSAFE_FLAG)) && (!scan_pages(mmu_get_top_table(), heap_current, count, true))){
        return NULL;
    }

    uintptr_t max_addr = heap_current + ((count - 1) * mmu_layout->levels[0].page_size);
    void* ptr = (void*)heap_current;
    if (!alloc_ki_pages(hhdm_request.response->offset + mmu_get_top_table(), mmu_layout->levels_count - 1, heap_current, max_addr, false, flags & NX_FLAG)){
        return NULL;
    }

    heap_current += count * mmu_layout->levels[0].page_size;
    return ptr;
}

void* alloc_user_pages(uintptr_t top_table, uint64_t user_vaddr, uint64_t count, uint64_t flags){
    if (count == 0 || allocated_pages + count > total_pages || user_vaddr + ((count - 1) * 0x1000) >= 0x0000800000000000) {
        return NULL;
    }

    if ((!(flags & UNSAFE_FLAG)) && (!scan_pages(top_table, user_vaddr, count, true))){
        return NULL;
    }

    uintptr_t max_addr = user_vaddr + ((count - 1) * mmu_layout->levels[0].page_size);
    if (!alloc_ki_pages(hhdm_request.response->offset + top_table, mmu_layout->levels_count - 1, user_vaddr, max_addr, true, flags & NX_FLAG)){
        return NULL;
    }

    return (void*)user_vaddr;
    // void* ptr = alloc_ki_pages(user_vaddr, count, true, flags);
    // return ptr;
}



bool map_ki_hardware(uintptr_t table_ptr, uint8_t level, uintptr_t curr_addr, uintptr_t max_addr, uintptr_t *curr_pfn, bool user, bool readonly){
    page_table_entry table_entry;
    int64_t span = 1ull << mmu_layout->levels[level].shift;
    while (curr_addr <= max_addr) {
        mmu_get_elem_info(table_ptr, curr_addr, level, &table_entry);
        if (!table_entry.present){
            if (level == 0){
                //map page
                table_entry.address = *curr_pfn;
                (*curr_pfn)++;
                table_entry.user = user;
                table_entry.writable = !readonly;
                table_entry.present = 1;
                mmu_set_elem_info(table_ptr, curr_addr, level, &table_entry);
                mmu_update_addr(curr_addr);
                curr_addr += span;
                continue;
            } else {
                //add next level
                struct Page* new_table_page = alloc_pages(0);
                if (!new_table_page) panic();

                uint64_t new_table_phys = (new_table_page - pages_map) << mmu_layout->levels[0].shift;
                clear_page_table((void*)(hhdm_request.response->offset + new_table_phys));

                table_entry.address = new_table_phys >> mmu_layout->levels[0].shift;
                table_entry.user = user;
                table_entry.writable = 1;
                table_entry.present = 1;
                mmu_set_elem_info(table_ptr, curr_addr, level, &table_entry);
            }
        }

        if (level == 0){
            (*curr_pfn)++;
            curr_addr += span;
            continue;
        } 

        
        uint64_t smax_addr = MIN(max_addr, curr_addr | (span - 1));
        if (!map_ki_hardware((table_entry.address << mmu_layout->levels[0].shift) + hhdm_request.response->offset, level - 1, curr_addr, smax_addr, curr_pfn, user, readonly)){
            return false;
        }

        curr_addr = smax_addr + 1;
    }
    return true;
}

uintptr_t map_hardware_user(uintptr_t top_table, process_t* process, uintptr_t user_vaddr, uint64_t count, uintptr_t phys_page, uint64_t mem_type, uint64_t flags){
    // return (uint64_t)map_ki_hardware(user_vaddr, count, phys_page, mem_type, flags, 1);
    uintptr_t max_addr = user_vaddr + ((count - 1) * mmu_layout->levels[0].page_size);

    if (!(process->rights & PROC_DRIVER_RIGHT)){
        return 0;
    }

    switch (mem_type) {
        case LIMINE_MEMMAP_RESERVED_MAPPED:
        case LIMINE_MEMMAP_RESERVED:
            if (!(process->rights & PROC_MEM_RESERVED_RIGHT))
                return 0;
            break;
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
            if (!(process->rights & PROC_MEM_ACPI_TABLE_RIGHT))
                return 0;
            break;
        case LIMINE_MEMMAP_ACPI_NVS:
            if (!(process->rights & PROC_MEM_ACPI_NVS_RIGHT))
                return 0;
            break;
        case LIMINE_MEMMAP_FRAMEBUFFER:
            if (!(process->rights & PROC_MEM_FRAMEBUFFER_RIGHT))
                return 0;
            break;
        default:
            return 0;
    }



    bool readonly = false;
    if (mem_type == LIMINE_MEMMAP_ACPI_RECLAIMABLE || flags & READONLY_FLAG){
        readonly = true;
    }

    
    if (user_vaddr + (count * 0x1000) >= 0x0000800000000000) {
        return 0;
    }


    if ((!(flags & UNSAFE_FLAG)) && (!scan_pages(top_table, user_vaddr, count, true))){
        return 0;
    }

    uint64_t start_sel = 0;
    bool found = false;
    for (uint64_t x = 0; x < memmmap_request.response->entry_count; x++){
        if (memmmap_request.response->entries[x]->base >> mmu_layout->levels[0].shift <= phys_page && (memmmap_request.response->entries[x]->base + memmmap_request.response->entries[x]->length) >> mmu_layout->levels[0].shift >= phys_page + count){
            start_sel = x;
            found = true;
            break;
        }
    }

    if (memmmap_request.response->entries[start_sel]->type != mem_type || !found){
        return 0;
    }



    map_ki_hardware(hhdm_request.response->offset + top_table, mmu_layout->levels_count - 1, user_vaddr, max_addr, &phys_page, true, readonly);
    return user_vaddr;
}

uintptr_t mmu_create_top_table(){
    struct Page* phys_page = alloc_pages(0);
    if (!phys_page) panic();

    uintptr_t x1 = mmu_layout->levels[mmu_layout->levels_count - 1].entries_count / 2;
    uintptr_t x2 = mmu_layout->levels[mmu_layout->levels_count - 1].entries_count;

    uintptr_t paddr = hhdm_request.response->offset + ((phys_page - pages_map) * 0x1000);

    for (uintptr_t x = 0; x < x1; x++){
        ((uintptr_t*)paddr)[x] = 0;
    }

    for (uintptr_t x = x1; x < x2; x++){
        ((uintptr_t*)paddr)[x] = ((uintptr_t*)(hhdm_request.response->offset + mmu_get_top_table()))[x];
    }

    return ((phys_page - pages_map) * 0x1000);
}


uint64_t alloc_kernel_stack(){
    struct Page* phys_page = alloc_pages(2);
    if (!phys_page) panic();

    uint64_t paddr = hhdm_request.response->offset + ((phys_page - pages_map) * 0x1000);
    return paddr;
}


typedef struct alloc_t {
    uint64_t magic; //0xDEADBEEF
    struct arena_t* arena;
    uint64_t size;
    struct alloc_t* prev;
    struct alloc_t* next;
} __attribute__((aligned(16))) alloc_t;

typedef struct arena_t {
    uint64_t magic; //0xDEADF00D
    uint64_t order;
    uint64_t allocations;
    alloc_t* first;
    uint64_t largest_free;
    uint64_t free_space;
    struct arena_t* prev;
    struct arena_t* next;
} __attribute__((aligned(16))) arena_t;

arena_t* first_arena = NULL;

#define ALIGN_UP(num, base) ((num) + (((base) - ((num) % (base))) % (base)))

void* kernel_malloc(uint64_t size){
    uint64_t real_size = ALIGN_UP(size + sizeof(alloc_t), 16);

    if (first_arena == NULL){
        uint8_t order = clog2((real_size + sizeof(arena_t) + (1 << mmu_layout->levels[0].shift) - 1) >> mmu_layout->levels[0].shift);
        struct Page* phys_page = alloc_pages(order);
        if (phys_page == NULL)
            return NULL;
        uint64_t paddr = (phys_page - pages_map);
        struct arena_t* arena = (void*)(hhdm_request.response->offset + (paddr << mmu_layout->levels[0].shift));
        arena->order = order;
        arena->magic = 0xDEADF00D;
        arena->allocations = 0;
        arena->first = NULL;
        arena->largest_free = (1ULL << (order + mmu_layout->levels[0].shift)) - sizeof(arena_t);
        arena->free_space = arena->largest_free;
        arena->prev = NULL;
        arena->next = NULL;
        first_arena = arena;
    }

    struct arena_t* current_arena = first_arena;
    bool flag = false;
    while (true) {
        if (current_arena->largest_free >= real_size){
            flag = true;
            break;
        }
        if (current_arena->next != NULL){
            current_arena = current_arena->next;
        } else {
            break;
        }
    }

    if (flag == false){
        uint8_t order = clog2((real_size + sizeof(arena_t) + (1 << mmu_layout->levels[0].shift) - 1) >> mmu_layout->levels[0].shift);
        struct Page* phys_page = alloc_pages(order);
        if (phys_page == NULL)
            return NULL;
        uint64_t paddr = (phys_page - pages_map);
        struct arena_t* arena = (void*)(hhdm_request.response->offset + (paddr << mmu_layout->levels[0].shift));
        arena->order = order;
        arena->magic = 0xDEADF00D;
        arena->allocations = 0;
        arena->first = NULL;
        arena->largest_free = (1ULL << (order + mmu_layout->levels[0].shift)) - sizeof(arena_t);
        arena->free_space = arena->largest_free;
        arena->prev = current_arena;
        arena->next = NULL;
        if (current_arena != NULL){
            current_arena->next = arena;
        }
        current_arena = arena;
    }
    alloc_t* current_alloc_block = current_arena->first;
    alloc_t* new_alloc_block = NULL;

    while (true) {
        if (current_alloc_block == NULL){ 
            // first elem
            new_alloc_block = (void*)((uint8_t*)current_arena + sizeof(arena_t));
            new_alloc_block->prev = NULL;
            new_alloc_block->next = NULL;
            current_arena->first = new_alloc_block;
            break;
        } else if (current_arena->first == current_alloc_block && (uint64_t)((uint8_t*)current_arena->first - ((uint8_t*)current_arena + sizeof(arena_t))) >= real_size){
            // before first
            new_alloc_block = (void*)((uint8_t*)current_arena + sizeof(arena_t));
            new_alloc_block->prev = NULL;
            new_alloc_block->next = current_arena->first;
            current_arena->first = new_alloc_block;
            new_alloc_block->next->prev = new_alloc_block;
            break;
        } else if (current_alloc_block->next == NULL){
            // at the end
            new_alloc_block = (void*)((uint8_t*)current_alloc_block + current_alloc_block->size);
            current_alloc_block->next = new_alloc_block;
            new_alloc_block->prev = current_alloc_block;
            new_alloc_block->next = NULL;
            break;
        } else if ((uint64_t)((uint8_t*)current_alloc_block->next - ((uint8_t*)current_alloc_block + current_alloc_block->size)) >= real_size){
            // between elems
            new_alloc_block = (void*)((uint8_t*)current_alloc_block + current_alloc_block->size);
            current_alloc_block->next->prev = new_alloc_block;
            new_alloc_block->next = current_alloc_block->next;
            new_alloc_block->prev = current_alloc_block;
            current_alloc_block->next = new_alloc_block;
            break;
        }

        current_alloc_block = current_alloc_block->next;
    }

    new_alloc_block->size = real_size;
    new_alloc_block->arena = current_arena;
    new_alloc_block->magic = 0xDEADBEEF;
    current_arena->allocations++;
    current_arena->free_space -= real_size;
    
    current_alloc_block = current_arena->first;

    uint64_t largest_free = (uint64_t)current_alloc_block - ((uint64_t)current_arena + sizeof(arena_t));
    while (true) {
        if (current_alloc_block->next != NULL){
            largest_free = MAX(largest_free, (uint64_t)current_alloc_block->next - ((uint64_t)current_alloc_block + current_alloc_block->size));
            current_alloc_block = current_alloc_block->next;
        } else {
            largest_free = MAX(largest_free,  (1ULL << (current_arena->order + mmu_layout->levels[0].shift)) - ((uint64_t)current_alloc_block + current_alloc_block->size - (uint64_t)current_arena));
            break;
        }
    }
    current_arena->largest_free = largest_free;
    

    return (uint8_t*)new_alloc_block + sizeof(alloc_t);
}


void kernel_free(void* addr){
    if (addr == NULL){
        return;
    }
    alloc_t* object = (void*)((uint8_t*)addr - sizeof(alloc_t));
    if (object->magic != 0xDEADBEEF){
        return;
    }
    object->magic = 0;

    if (object->prev != NULL) {
        object->prev->next = object->next;
    } else {
        object->arena->first = object->next;
    }

    if (object->next != NULL) {
        object->next->prev = object->prev;
    }

    arena_t* arena = object->arena;
    if (arena->magic != 0xDEADF00D){
        panic();
    }
    arena->allocations--;
    arena->free_space += object->size;

    if (arena->allocations == 0){
        if (arena->prev != NULL){
            arena->prev->next = arena->next;
        } else {
            first_arena = arena->next;
        }
        
        if (arena->next != NULL){
            arena->next->prev = arena->prev;
        }
        arena->magic = 0;
        free_buddy_pages(&pages_map[((uint64_t)arena - hhdm_request.response->offset) >> mmu_layout->levels[0].shift], arena->order);
    } else {

        alloc_t* current_alloc_block = arena->first;

        uint64_t largest_free = (uint64_t)current_alloc_block - ((uint64_t)arena + sizeof(arena_t));
        while (true) {
            if (current_alloc_block->next != NULL){
                largest_free = MAX(largest_free, (uint64_t)current_alloc_block->next - ((uint64_t)current_alloc_block + current_alloc_block->size));
                current_alloc_block = current_alloc_block->next;
            } else {
                largest_free = MAX(largest_free,  (1ULL << (arena->order + mmu_layout->levels[0].shift)) - ((uint64_t)current_alloc_block + current_alloc_block->size - (uint64_t)arena));
                break;
            }
        }
        arena->largest_free = largest_free;
    }
}