// SPDX-License-Identifier: GPL-2.0-or-later
#include "paging.h"
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <stdint.h>
#include <limine_requests.h>
#include <snprintf.h>
#include "../debug/fb.h"
#include <math.h>
#include <panic.h>
#include <tasks.h>
#include <abstractions.h>
#include <stdlib.h>


struct Page* buddy_alloc_ptr[MAX_ORDER];
struct Page* pages_map = NULL;
uint64_t total_pages = 0;
uint64_t heap_current = HEAP_ADDR;
uint64_t allocated_pages = 0;

#define VIRT_ADDR(pml4, pdpt, pd, pt) \
    ((((uint64_t)(pml4) & 0x1FF) << 39) | \
     (((uint64_t)(pdpt) & 0x1FF) << 30) | \
     (((uint64_t)(pd)   & 0x1FF) << 21) | \
     (((uint64_t)(pt)   & 0x1FF) << 12) | \
     (((pml4) & 0x100) ? 0xFFFF000000000000ULL : 0))
#define PML4_INDEX(virt_ptr) (((virt_ptr) >> 39) & 0x1FF)
#define PDPT_INDEX(virt_ptr) (((virt_ptr) >> 30) & 0x1FF)
#define PD_INDEX(virt_ptr)   (((virt_ptr) >> 21) & 0x1FF)
#define PT_INDEX(virt_ptr)   (((virt_ptr) >> 12) & 0x1FF)

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
    uint64_t pml4_physical;
    asm volatile("mov %%cr3, %0" : "=r"(pml4_physical));
    uint64_t hhdm_offset = hhdm_request.response->offset;

    uint64_t mem_size = memmap->entries[memmap->entry_count - 1]->base + memmap->entries[memmap->entry_count - 1]->length;
    total_pages = mem_size / 4096;
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
            if (memmap->entries[x]->base >> 12 <= i && (memmap->entries[x]->base + memmap->entries[x]->length) >> 12 > i){
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

    uint64_t metadata_start_pfn = base_addr / 4096;
    uint64_t metadata_end_pfn = (base_addr + pages_map_size + 4095) / 4096;
    uint64_t free_pages = 0;
    for (uint64_t x = 0; x < memmap->entry_count; x++) {
        if (memmap->entries[x]->type != LIMINE_MEMMAP_USABLE) continue;

        uint64_t start_pfn = memmap->entries[x]->base / 4096;
        uint64_t end_pfn = (memmap->entries[x]->base + memmap->entries[x]->length) / 4096;

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




void* alloc_kernel_pages(uint64_t count, uint64_t flags){
    void* ptr = alloc_ki_pages(heap_current, count, false, flags);
    if (ptr == NULL){
        return NULL;
    }

    heap_current += count * 0x1000;
    return ptr;
}

void* alloc_user_pages(uint64_t user_vaddr, uint64_t count, uint64_t flags){
    void* ptr = alloc_ki_pages(user_vaddr, count, true, flags);
    return ptr;
}

bool scan_pages(uint64_t vaddr, uint64_t count, uint64_t pml4_phys_ptr, bool searchtoalloc){
    uint64_t start_addr = vaddr;
    uint64_t end_addr = vaddr + ((count - 1) * 0x1000);

    uint64_t hhdm_offset = hhdm_request.response->offset;
    page_tree tree;

    page_table_entry* pml4_table = (page_table_entry*)(hhdm_offset + pml4_phys_ptr);


    for (uint64_t a = PML4_INDEX(start_addr); a < PML4_INDEX(end_addr) + 1; a++){
        tree.pml4 = pml4_table[a];
        if (!tree.pml4.present && !searchtoalloc) return false;
        if (!tree.pml4.present && searchtoalloc) continue;                    

        page_table_entry* pdpt_table = (page_table_entry*)(hhdm_offset + (tree.pml4.address << 12));

        for (uint64_t b = (a == PML4_INDEX(start_addr) ? PDPT_INDEX(start_addr) : 0); b < (a == PML4_INDEX(end_addr) ? PDPT_INDEX(end_addr) + 1 : 512); b++){
            tree.pdpt = pdpt_table[b];
            if (!tree.pdpt.present && !searchtoalloc) return false;
            if (!tree.pdpt.present && searchtoalloc) continue;                    

            page_table_entry* pd_table = (page_table_entry*)(hhdm_offset + (tree.pdpt.address << 12));

            for (uint64_t c = (a == PML4_INDEX(start_addr) && b == PDPT_INDEX(start_addr) ? PD_INDEX(start_addr) : 0); c < (a == PML4_INDEX(end_addr) && b == PDPT_INDEX(end_addr) ? PD_INDEX(end_addr) + 1 : 512); c++){
                tree.pd = pd_table[c];
                if (!tree.pd.present && !searchtoalloc) return false;
                if (!tree.pd.present && searchtoalloc) continue;                    

                page_table_entry* pt_table = (page_table_entry*)(hhdm_offset + (tree.pd.address << 12));
                for (uint64_t d = (a == PML4_INDEX(start_addr) && b == PDPT_INDEX(start_addr) && c == PD_INDEX(start_addr) ? PT_INDEX(start_addr) : 0); d < (a == PML4_INDEX(end_addr) && b == PDPT_INDEX(end_addr) && c == PD_INDEX(end_addr) ? PT_INDEX(end_addr) + 1 : 512); d++){
                    tree.pt = pt_table[d];
                    if (!tree.pt.present && !searchtoalloc) return false;
                    if (tree.pt.present && searchtoalloc) return false;                    

                }

            }

        }
    }

    return true;
    
}


int64_t free_ki_pages(uint64_t user_vaddr, uint64_t count, bool user, uint64_t flags){
    uint64_t pml4_physical;
    asm volatile("mov %%cr3, %0" : "=r"(pml4_physical));
    
    if (count == 0 || allocated_pages < count){
        return -1;
    }
    if ((user && user_vaddr + (count * 0x1000) >= 0x0000800000000000) || (!user && user_vaddr < 0x0000800000000000)) {
        return -1;
    }


    if (!(flags & UNSAFE_FLAG)){
        if (!scan_pages(user_vaddr, count, pml4_physical, false)){
            return -1;
        }

    }


    uint64_t start_addr = user_vaddr;
    uint64_t end_addr = start_addr + ((count - 1) * 0x1000);
    uint64_t hhdm_offset = hhdm_request.response->offset;

    page_table_entry* pml4_table = (page_table_entry*)(hhdm_offset + pml4_physical);

    for (uint64_t a = PML4_INDEX(start_addr); a < PML4_INDEX(end_addr) + 1; a++) {
        if (!pml4_table[a].present) continue;
        page_table_entry* pdpt_table = (page_table_entry*)(hhdm_offset + (pml4_table[a].address << 12));

        for (uint64_t b = (a == PML4_INDEX(start_addr) ? PDPT_INDEX(start_addr) : 0); b < (a == PML4_INDEX(end_addr) ? PDPT_INDEX(end_addr) + 1 : 512); b++) {
            if (!pdpt_table[b].present) continue;

            page_table_entry* pd_table = (page_table_entry*)(hhdm_offset + (pdpt_table[b].address << 12));

            for (uint64_t c = (a == PML4_INDEX(start_addr) && b == PDPT_INDEX(start_addr) ? PD_INDEX(start_addr) : 0); c < (a == PML4_INDEX(end_addr) && b == PDPT_INDEX(end_addr) ? PD_INDEX(end_addr) + 1 : 512); c++) {
                if (!pd_table[c].present) continue;
                page_table_entry* pt_table = (page_table_entry*)(hhdm_offset + (pd_table[c].address << 12));
                
                uint64_t d_start = (a == PML4_INDEX(start_addr) && b == PDPT_INDEX(start_addr) && c == PD_INDEX(start_addr) ? PT_INDEX(start_addr) : 0);
                uint64_t d_end = (a == PML4_INDEX(end_addr) && b == PDPT_INDEX(end_addr) && c == PD_INDEX(end_addr) ? PT_INDEX(end_addr) + 1 : 512);

                for (uint64_t d = d_start; d < d_end; d++) {
                    if (pt_table[d].present) { //todo: change to flags check when swap will be realized

                        uint64_t page = pt_table[d].address;
                        if (pages_map[page].flags & PAGE_FREE){
                            panic();
                        }
                        //todo: remove free mem tables if entire region is free
                        pt_table[d].present = 0;
                        pt_table[d].address = 0;                        
                        asm volatile("invlpg (%0)" :: "r"(VIRT_ADDR(a, b, c, d)) : "memory");

                        if (!(pages_map[page].flags & PAGE_HARDWARE)){
                            pages_map[page]._allocated.counters--;

                            if (pages_map[page]._allocated.counters == 0){
                                free_buddy_pages(&pages_map[page], 0);
                            }
                        }


                    }
                }
            }
        }
    }
    return 0;
}

int64_t free_user_pages(uint64_t user_vaddr, uint64_t count, uint64_t flags){
    return free_ki_pages(user_vaddr, count, 1, flags);
}


int64_t free_kernel_pages(uint64_t user_vaddr, uint64_t count, uint64_t flags){
    return free_ki_pages(user_vaddr, count, 0, flags);
}


void* alloc_ki_pages(uint64_t user_vaddr, uint64_t count, bool user, uint64_t flags) {
    uint64_t pml4_physical;
    asm volatile("mov %%cr3, %0" : "=r"(pml4_physical));

    if (count == 0 || allocated_pages + count > total_pages){
        return NULL;
    }
    if ((user && user_vaddr + (count * 0x1000) >= 0x0000800000000000) || (!user && user_vaddr < 0x0000800000000000)) {
        return NULL;
    }

    if (!(flags & UNSAFE_FLAG)){
        if (!scan_pages(user_vaddr, count, pml4_physical, true)){
            return NULL;
        }

    }


    uint64_t start_addr = user_vaddr;
    uint64_t end_addr = start_addr + ((count - 1) * 0x1000);
    uint64_t hhdm_offset = hhdm_request.response->offset;

    page_table_entry* pml4_table = (page_table_entry*)(hhdm_offset + pml4_physical);

    for (uint64_t a = PML4_INDEX(start_addr); a < PML4_INDEX(end_addr) + 1; a++) {
        if (!pml4_table[a].present) {
            struct Page* new_table_page = alloc_pages(0);
            if (!new_table_page) panic();

            uint64_t new_table_phys = (new_table_page - pages_map) * 4096;
            clear_page_table((void*)(hhdm_offset + new_table_phys));

            pml4_table[a].present = 1;
            pml4_table[a].writable = 1;
            pml4_table[a].user = user;
            pml4_table[a].address = new_table_phys >> 12;
        }
        page_table_entry* pdpt_table = (page_table_entry*)(hhdm_offset + (pml4_table[a].address << 12));

        for (uint64_t b = (a == PML4_INDEX(start_addr) ? PDPT_INDEX(start_addr) : 0); b < (a == PML4_INDEX(end_addr) ? PDPT_INDEX(end_addr) + 1 : 512); b++) {
            if (!pdpt_table[b].present) {
                struct Page* new_table_page = alloc_pages(0);
                if (!new_table_page) panic();

                uint64_t new_table_phys = (new_table_page - pages_map) * 4096;
                clear_page_table((void*)(hhdm_offset + new_table_phys));

                pdpt_table[b].present = 1;
                pdpt_table[b].writable = 1;
                pdpt_table[b].user = user;
                pdpt_table[b].address = new_table_phys >> 12;
            }
            page_table_entry* pd_table = (page_table_entry*)(hhdm_offset + (pdpt_table[b].address << 12));

            for (uint64_t c = (a == PML4_INDEX(start_addr) && b == PDPT_INDEX(start_addr) ? PD_INDEX(start_addr) : 0); c < (a == PML4_INDEX(end_addr) && b == PDPT_INDEX(end_addr) ? PD_INDEX(end_addr) + 1 : 512); c++) {
                if (!pd_table[c].present) {
                    struct Page* new_table_page = alloc_pages(0);
                    if (!new_table_page) panic();

                    uint64_t new_table_phys = (new_table_page - pages_map) * 4096;
                    clear_page_table((void*)(hhdm_offset + new_table_phys));

                    pd_table[c].present = 1;
                    pd_table[c].writable = 1;
                    pd_table[c].user = user;
                    pd_table[c].address = new_table_phys >> 12;
                }
                page_table_entry* pt_table = (page_table_entry*)(hhdm_offset + (pd_table[c].address << 12));
                
                uint64_t d_start = (a == PML4_INDEX(start_addr) && b == PDPT_INDEX(start_addr) && c == PD_INDEX(start_addr) ? PT_INDEX(start_addr) : 0);
                uint64_t d_end = (a == PML4_INDEX(end_addr) && b == PDPT_INDEX(end_addr) && c == PD_INDEX(end_addr) ? PT_INDEX(end_addr) + 1 : 512);

                for (uint64_t d = d_start; d < d_end; d++) {
                    if (!pt_table[d].present) {
                        struct Page* phys_page = alloc_pages(0);
                        if (!phys_page) panic();

                        uint64_t paddr = (phys_page - pages_map);
                        
                        pt_table[d].address = paddr;
                        pt_table[d].user = user;
                        pt_table[d].writable = 1;
                        pt_table[d].present = 1;

                        asm volatile("invlpg (%0)" :: "r"(VIRT_ADDR(a, b, c, d)) : "memory");
                    }
                }
            }
        }
    }
    

    return (void*)user_vaddr;
}


void* map_ki_hardware(uint64_t user_vaddr,	uint64_t count, uint64_t phys_page, uint64_t mem_type, uint64_t flags, bool user){
    uint64_t pml4_physical;
    asm volatile("mov %%cr3, %0" : "=r"(pml4_physical));

    task_control_block_t* tcb = get_thread_info_ptr();
    if (!(tcb->process->rights & PROC_DRIVER_RIGHT)){
        return NULL;
    }

    switch (mem_type) {
        case LIMINE_MEMMAP_RESERVED:
            if (!(tcb->process->rights & PROC_MEM_RESERVED_RIGHT))
                return NULL;
            break;
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
            if (!(tcb->process->rights & PROC_MEM_ACPI_TABLE_RIGHT))
                return NULL;
            break;
        case LIMINE_MEMMAP_ACPI_NVS:
            if (!(tcb->process->rights & PROC_MEM_ACPI_NVS_RIGHT))
                return NULL;
            break;
        case LIMINE_MEMMAP_FRAMEBUFFER:
            if (!(tcb->process->rights & PROC_MEM_FRAMEBUFFER_RIGHT))
                return NULL;
            break;
        case LIMINE_MEMMAP_RESERVED_MAPPED:
            if (!(tcb->process->rights & PROC_MEM_RESERVER_MAPPED_RIGHT))
                return NULL;
            break;
        default:
            return 0;
    }



    bool readonly = false;
    if (mem_type == LIMINE_MEMMAP_ACPI_RECLAIMABLE || flags & READONLY_FLAG){
        readonly = true;
    }

    
    if ((user && user_vaddr + (count * 0x1000) >= 0x0000800000000000) || (!user && user_vaddr < 0x0000800000000000)) {
        return NULL;
    }

    if (!scan_pages(user_vaddr, count, pml4_physical, true)){
        return NULL;
    }


    uint64_t start_sel = 0;
    bool found = false;
    for (uint64_t x = 0; x < memmmap_request.response->entry_count; x++){
        if (memmmap_request.response->entries[x]->base >> 12 <= phys_page && (memmmap_request.response->entries[x]->base + memmmap_request.response->entries[x]->length) >> 12 >= phys_page + count){
            start_sel = x;
            found = true;
            break;
        }
    }

    if (memmmap_request.response->entries[start_sel]->type != mem_type || !found){
        return NULL;
    }


    uint64_t start_addr = user_vaddr;
    uint64_t end_addr = start_addr + ((count - 1) * 0x1000);
    uint64_t hhdm_offset = hhdm_request.response->offset;

    page_table_entry* pml4_table = (page_table_entry*)(hhdm_offset + pml4_physical);
    uint64_t curr_pfn = phys_page;

    for (uint64_t a = PML4_INDEX(start_addr); a < PML4_INDEX(end_addr) + 1; a++) {
        if (!pml4_table[a].present) {
            struct Page* new_table_page = alloc_pages(0);
            if (!new_table_page) panic();

            uint64_t new_table_phys = (new_table_page - pages_map) * 4096;
            clear_page_table((void*)(hhdm_offset + new_table_phys));

            pml4_table[a].present = 1;
            pml4_table[a].writable = 1;
            pml4_table[a].user = user;
            pml4_table[a].address = new_table_phys >> 12;
        }
        page_table_entry* pdpt_table = (page_table_entry*)(hhdm_offset + (pml4_table[a].address << 12));

        for (uint64_t b = (a == PML4_INDEX(start_addr) ? PDPT_INDEX(start_addr) : 0); b < (a == PML4_INDEX(end_addr) ? PDPT_INDEX(end_addr) + 1 : 512); b++) {
            if (!pdpt_table[b].present) {
                struct Page* new_table_page = alloc_pages(0);
                if (!new_table_page) panic();

                uint64_t new_table_phys = (new_table_page - pages_map) * 4096;
                clear_page_table((void*)(hhdm_offset + new_table_phys));

                pdpt_table[b].present = 1;
                pdpt_table[b].writable = 1;
                pdpt_table[b].user = user;
                pdpt_table[b].address = new_table_phys >> 12;
            }
            page_table_entry* pd_table = (page_table_entry*)(hhdm_offset + (pdpt_table[b].address << 12));

            for (uint64_t c = (a == PML4_INDEX(start_addr) && b == PDPT_INDEX(start_addr) ? PD_INDEX(start_addr) : 0); c < (a == PML4_INDEX(end_addr) && b == PDPT_INDEX(end_addr) ? PD_INDEX(end_addr) + 1 : 512); c++) {
                if (!pd_table[c].present) {
                    struct Page* new_table_page = alloc_pages(0);
                    if (!new_table_page) panic();

                    uint64_t new_table_phys = (new_table_page - pages_map) * 4096;
                    clear_page_table((void*)(hhdm_offset + new_table_phys));

                    pd_table[c].present = 1;
                    pd_table[c].writable = 1;
                    pd_table[c].user = user;
                    pd_table[c].address = new_table_phys >> 12;
                }
                page_table_entry* pt_table = (page_table_entry*)(hhdm_offset + (pd_table[c].address << 12));
                
                uint64_t d_start = (a == PML4_INDEX(start_addr) && b == PDPT_INDEX(start_addr) && c == PD_INDEX(start_addr) ? PT_INDEX(start_addr) : 0);
                uint64_t d_end = (a == PML4_INDEX(end_addr) && b == PDPT_INDEX(end_addr) && c == PD_INDEX(end_addr) ? PT_INDEX(end_addr) + 1 : 512);

                for (uint64_t d = d_start; d < d_end; d++) {
                    if (!pt_table[d].present) {
   
                        
                        pt_table[d].address = curr_pfn;
                        pt_table[d].user = user;
                        pt_table[d].writable = !readonly;
                        pt_table[d].present = 1;

                        asm volatile("invlpg (%0)" :: "r"(VIRT_ADDR(a, b, c, d)) : "memory");
                    }
                    curr_pfn++;
                }
            }
        }
    }
    return (void*)user_vaddr;
}

uint64_t map_hardware_user(uint64_t user_vaddr,	uint64_t count, uint64_t phys_page, uint64_t mem_type, uint64_t flags){
    return (uint64_t)map_ki_hardware(user_vaddr, count, phys_page, mem_type, flags, 1);
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
        uint8_t order = clog2((real_size + sizeof(arena_t) + 4095) >> 12);
        struct Page* phys_page = alloc_pages(order);
        if (phys_page == NULL)
            return NULL;
        uint64_t paddr = (phys_page - pages_map);
        struct arena_t* arena = (void*)(hhdm_request.response->offset + (paddr << 12));
        arena->order = order;
        arena->magic = 0xDEADF00D;
        arena->allocations = 0;
        arena->first = NULL;
        arena->largest_free = (1ULL << (order + 12)) - sizeof(arena_t);
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
        uint8_t order = clog2((real_size + sizeof(arena_t) + 4095) >> 12);
        struct Page* phys_page = alloc_pages(order);
        if (phys_page == NULL)
            return NULL;
        uint64_t paddr = (phys_page - pages_map);
        struct arena_t* arena = (void*)(hhdm_request.response->offset + (paddr << 12));
        arena->order = order;
        arena->magic = 0xDEADF00D;
        arena->allocations = 0;
        arena->first = NULL;
        arena->largest_free = (1ULL << (order + 12)) - sizeof(arena_t);
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
            largest_free = MAX(largest_free,  (1ULL << (current_arena->order + 12)) - ((uint64_t)current_alloc_block + current_alloc_block->size - (uint64_t)current_arena));
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
        free_buddy_pages(&pages_map[((uint64_t)arena - hhdm_request.response->offset) >> 12], arena->order);
    } else {

        alloc_t* current_alloc_block = arena->first;

        uint64_t largest_free = (uint64_t)current_alloc_block - ((uint64_t)arena + sizeof(arena_t));
        while (true) {
            if (current_alloc_block->next != NULL){
                largest_free = MAX(largest_free, (uint64_t)current_alloc_block->next - ((uint64_t)current_alloc_block + current_alloc_block->size));
                current_alloc_block = current_alloc_block->next;
            } else {
                largest_free = MAX(largest_free,  (1ULL << (arena->order + 12)) - ((uint64_t)current_alloc_block + current_alloc_block->size - (uint64_t)arena));
                break;
            }
        }
        arena->largest_free = largest_free;
    }
}