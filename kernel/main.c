// SPDX-License-Identifier: GPL-2.0-or-later
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "cpu.h"
#include "debug.h"
#include "include/tasks.h"
#include "mmu.h"
#include "paging.h"
#include <snprintf.h>
#include <string.h>
#include <tasks.h>
#include <limine_requests.h>
#include <syscall.h>
#include <panic.h>
#include "bootstrap/elfloader/elf.h"
#include <cpui.h>


// Halt and catch fire function.

void test_allocator() {
    char buf[256];

    logk("--- STARTING MEMORY ALLOCATOR TEST ---");

    // Step 1: Check initial heap boundary
    snprintf(buf, sizeof(buf), "Initial heap address: 0x%llx", (unsigned long long)heap_current);
    logk(buf);

    // Step 2: Allocate the first page (Should land exactly at HEAP_ADDR)
    uint64_t* ptr1 = (uint64_t*)alloc_kernel_pages(1, 0);
    if (ptr1 == NULL) {
        logk("[CRITICAL] Failed to allocate page 1 (ptr1)!");
        return;
    }
    snprintf(buf, sizeof(buf), "Allocated page 1 (ptr1): 0x%llx", (unsigned long long)ptr1);
    logk(buf);

    // Step 3: Allocate the second page (Should step forward by exactly 4 KB / 0x1000)
    uint64_t* ptr2 = (uint64_t*)alloc_kernel_pages(1, 0);
    if (ptr2 == NULL) {
        logk("[CRITICAL] Failed to allocate page 2 (ptr2)!");
        return;
    }
    snprintf(buf, sizeof(buf), "Allocated page 2 (ptr2): 0x%llx", (unsigned long long)ptr2);
    logk(buf);

    uint64_t* ptr3 = (uint64_t*)alloc_kernel_pages(1, 0);
    if (ptr3 == NULL) {
        logk("[CRITICAL] Failed to allocate page 3 (ptr3)!");
        return;
    }
    snprintf(buf, sizeof(buf), "Allocated page 3 (ptr3): 0x%llx", (unsigned long long)ptr3);
    logk(buf);
    logk("Dealloc page 3 and alloc page 3");


    if (free_kernel_pages((uint64_t)ptr3, 1, 0) != 0) {
        logk("[CRITICAL] Failed to free page 3 (ptr3)!");
        return;
    }

    heap_current -= 4096;

    ptr3 = (uint64_t*)alloc_kernel_pages(1, 0);
    if (ptr3 == NULL) {
        logk("[CRITICAL] Failed to allocate page 3 (ptr3)!");
        return;
    }

    snprintf(buf, sizeof(buf), "Allocated page 3 (ptr3): 0x%llx", (unsigned long long)ptr3);
    logk(buf);

    uint64_t* ptr4 = (uint64_t*)alloc_user_pages(mmu_get_top_table(), 0x100000, 1000, 0);
    if (ptr4 == NULL) {
        logk("[CRITICAL] Failed to allocate user page 4 (ptr4)!");
        return;
    }
    snprintf(buf, sizeof(buf), "Allocated user page 4 (ptr4): 0x%llx", (unsigned long long)ptr4);
    logk(buf);


    logk("Attempting to write data to allocated virtual pages...");

    ptr1[0]   = 0xDEADBEEFCAFEBABE;
    ptr1[511] = 0x1122334455667788; // End of page 1

    ptr2[0]   = 0x8877665544321100;
    ptr2[511] = 0xABCDEF0123456789; // End of page 2

    ptr3[0]   = 0x8877665544321120;
    ptr3[511] = 0xABCDEF0123456729; // End of page 2
    ptr4[0]   = 0x8877665544321121;
    ptr4[511] = 0xABCDEF0123456721; // End of page 2

    logk("Writes completed safely. Verifying data integrity...");

    // Step 5: Readback verification
    if (ptr1[0] == 0xDEADBEEFCAFEBABE && ptr1[511] == 0x1122334455667788 &&
        ptr2[0] == 0x8877665544321100 && ptr2[511] == 0xABCDEF0123456789 &&
        ptr3[0] == 0x8877665544321120 && ptr3[511] == 0xABCDEF0123456729 &&
        ptr4[0] == 0x8877665544321121 && ptr4[511] == 0xABCDEF0123456721) {
        logk("[SUCCESS] Read/Write test passed cleanly. Data is structurally sound!");
    } else {
        logk("[ERROR] Critical failure: Data in heap is corrupted or cross-talking!");
    }

    // Step 6: Validate page traversal mapping engine via page_tree_get
    // uint64_t pml4_physical;
    // asm volatile("mov %%cr3, %0" : "=r"(pml4_physical));
    // // page_tree tree = page_tree_get(pml4_physical, (uint64_t)ptr1);
    // log("Inspecting the page table tree for ptr1:", framebuffer);

    // snprintf(buf, sizeof(buf), "  Deepest resolved level (Expected 1): %d", tree.min_level_exists);
    // log(buf, framebuffer);

    // snprintf(buf, sizeof(buf), "  PT entry Present bit (Expected 1): %d", tree.pt.present);
    // log(buf, framebuffer);

    // snprintf(buf, sizeof(buf), "  Physical PFN mapped: 0x%llx", (unsigned long long)tree.pt.address);
    // log(buf, framebuffer);

    if (free_user_pages(mmu_get_top_table(), (uint64_t)ptr4, 1, 0) != 0) {
        logk("[CRITICAL] Failed to free user page 4 (ptr4)!");
        return;
    }

    logk("--- MEMORY ALLOCATOR TEST COMPLETE ---");
}


// struct IretFrame
// {
//     uint64_t rip;
//     uint64_t cs;
//     uint64_t rflags;
//     uint64_t rsp;
//     uint64_t ss;
// } __attribute__((packed));


// #define USER_CS 0x1B
// #define USER_SS 0x23


void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    if (memmmap_request.response == NULL
     || memmmap_request.response->entry_count < 1) {
        hcf();
    }

    debug_init();



    logk("Detected memory:");
    for (uint64_t x = 0; x < memmmap_request.response->entry_count; x++){
        char buf[128];
        char *cur = buf;
        const char* end = buf + sizeof(buf);
        cur += snprintf(cur, end - cur, "Base: 0x%llx Size: %lluB Type: ", memmmap_request.response->entries[x]->base, memmmap_request.response->entries[x]->length);
        switch (memmmap_request.response->entries[x]->type) {
            case LIMINE_MEMMAP_USABLE:
                cur += snprintf(cur, end - cur, "USABLE");
                break;
            case LIMINE_MEMMAP_RESERVED:
                cur += snprintf(cur, end - cur, "RESERVED");
                break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
                cur += snprintf(cur, end - cur, "ACPI RECLAIMABLE");
                break;
            case LIMINE_MEMMAP_ACPI_NVS:
                cur += snprintf(cur, end - cur, "ACPI NVS");
                break;
            case LIMINE_MEMMAP_BAD_MEMORY:
                cur += snprintf(cur, end - cur, "BAD MEMORY");
                break;
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
                cur += snprintf(cur, end - cur, "BOOTLOADER RECLAIMABLE");
                break;
            case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
                cur += snprintf(cur, end - cur, "EXECUTABLES AND MODULES");
                break;
            case LIMINE_MEMMAP_FRAMEBUFFER:
                cur += snprintf(cur, end - cur, "FRAMEBUFFER");
                break;
            case LIMINE_MEMMAP_RESERVED_MAPPED:
                cur += snprintf(cur, end - cur, "RESERVED MAPPED");
                break;
        }
        logk(buf);
    }
    alloc_init(memmmap_request.response);
    ki_cpu_init();

    test_allocator();


    if(module_request.response->module_count == 0){
        logk("ERR");
    }


   



    int32_t proc = process_create();
    if (proc == -1){
        hcf();
    }

    uint64_t rip = (uint64_t)loadelf(process_getptr(proc), module_request.response->modules[0]->address, module_request.response->modules[0]->size);

    uint64_t addr = (uint64_t)alloc_user_pages(process_getptr(proc)->cr3, 0x1000000, 2, 0);

    if (addr == 0){
        logk("ERR");
        hcf();
    }

    thread_create(proc, (uint8_t*)rip, (uint8_t*)(addr + 0x2000), 0);

    process_ki_setrights(proc, PROC_DRIVER_RIGHT | PROC_MEM_FRAMEBUFFER_RIGHT);

    // task_control_block_t* task = kernel_malloc(sizeof(task_control_block_t));
    // process_t* proc = kernel_malloc(sizeof(process_t));
    // struct IretFrame frame = {
    //     .rip = rip,
    //     .cs = USER_CS,
    //     .rflags = 0x202,
    //     .rsp = addr + 0x2000,
    //     .ss = USER_SS,
    // };
    // (void)frame;
    // task->kernel_SP = alloc_kernel_stack();
    // task->user_IP = rip;
    // set_kernel_stack((void*)task->kernel_SP);
    // asm volatile(
    //     "mov %0, %%rsp\n\t"
    //     "iretq\n\t"
    //     :
    //     : "r"(&frame)
    //     : "memory"
    // );
    // logk("userspace_enter");
    // set_thread_info_ptr(task);
    // userspace_jmp(addr + 0x2000, task);
    start_sched();
    logk("exit");
    hcf();
}
