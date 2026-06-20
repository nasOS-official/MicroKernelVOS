#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "debug/fb.h"
#include "paging.h"
#include <snprintf.h>
#include <process.h>
#include <limine_requests.h>
#include <syscall.h>
#include <panic.h>

// Set the base revision to 6, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.
typedef struct color_t{
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
} color_t;


// Halt and catch fire function.

void test_allocator(struct limine_framebuffer *framebuffer) {
    char buf[256];

    log("--- STARTING MEMORY ALLOCATOR TEST ---", framebuffer);

    // Step 1: Check initial heap boundary
    snprintf(buf, sizeof(buf), "Initial heap address: 0x%llx", (unsigned long long)heap_current);
    log(buf, framebuffer);

    // Step 2: Allocate the first page (Should land exactly at HEAP_ADDR)
    uint64_t* ptr1 = (uint64_t*)alloc_kernel_pages(1, 0);
    if (ptr1 == NULL) {
        log("[CRITICAL] Failed to allocate page 1 (ptr1)!", framebuffer);
        return;
    }
    snprintf(buf, sizeof(buf), "Allocated page 1 (ptr1): 0x%llx", (unsigned long long)ptr1);
    log(buf, framebuffer);

    // Step 3: Allocate the second page (Should step forward by exactly 4 KB / 0x1000)
    uint64_t* ptr2 = (uint64_t*)alloc_kernel_pages(1, 0);
    if (ptr2 == NULL) {
        log("[CRITICAL] Failed to allocate page 2 (ptr2)!", framebuffer);
        return;
    }
    snprintf(buf, sizeof(buf), "Allocated page 2 (ptr2): 0x%llx", (unsigned long long)ptr2);
    log(buf, framebuffer);

    uint64_t* ptr3 = (uint64_t*)alloc_kernel_pages(1, 0);
    if (ptr3 == NULL) {
        log("[CRITICAL] Failed to allocate page 3 (ptr3)!", framebuffer);
        return;
    }
    snprintf(buf, sizeof(buf), "Allocated page 3 (ptr3): 0x%llx", (unsigned long long)ptr3);
    log(buf, framebuffer);
    log("Dealloc page 3 and alloc page 3", framebuffer);


    if (free_kernel_pages((uint64_t)ptr3, 1, 0) != 0) {
        log("[CRITICAL] Failed to free page 3 (ptr3)!", framebuffer);
        return;
    }

    heap_current -= 4096;

    ptr3 = (uint64_t*)alloc_kernel_pages(1, 0);
    if (ptr3 == NULL) {
        log("[CRITICAL] Failed to allocate page 3 (ptr3)!", framebuffer);
        return;
    }

    snprintf(buf, sizeof(buf), "Allocated page 3 (ptr3): 0x%llx", (unsigned long long)ptr3);
    log(buf, framebuffer);

    uint64_t* ptr4 = (uint64_t*)alloc_user_pages(0x100000, 1000, 0);
    if (ptr4 == NULL) {
        log("[CRITICAL] Failed to allocate user page 4 (ptr4)!", framebuffer);
        return;
    }
    snprintf(buf, sizeof(buf), "Allocated user page 4 (ptr4): 0x%llx", (unsigned long long)ptr4);
    log(buf, framebuffer);


    log("Attempting to write data to allocated virtual pages...", framebuffer);

    ptr1[0]   = 0xDEADBEEFCAFEBABE;
    ptr1[511] = 0x1122334455667788; // End of page 1

    ptr2[0]   = 0x8877665544321100;
    ptr2[511] = 0xABCDEF0123456789; // End of page 2

    ptr3[0]   = 0x8877665544321120;
    ptr3[511] = 0xABCDEF0123456729; // End of page 2
    ptr4[0]   = 0x8877665544321121;
    ptr4[511] = 0xABCDEF0123456721; // End of page 2

    log("Writes completed safely. Verifying data integrity...", framebuffer);

    // Step 5: Readback verification
    if (ptr1[0] == 0xDEADBEEFCAFEBABE && ptr1[511] == 0x1122334455667788 &&
        ptr2[0] == 0x8877665544321100 && ptr2[511] == 0xABCDEF0123456789 &&
        ptr3[0] == 0x8877665544321120 && ptr3[511] == 0xABCDEF0123456729 &&
        ptr4[0] == 0x8877665544321121 && ptr4[511] == 0xABCDEF0123456721) {
        log("[SUCCESS] Read/Write test passed cleanly. Data is structurally sound!", framebuffer);
    } else {
        log("[ERROR] Critical failure: Data in heap is corrupted or cross-talking!", framebuffer);
    }

    // Step 6: Validate page traversal mapping engine via page_tree_get
    uint64_t pml4_physical;
    asm volatile("mov %%cr3, %0" : "=r"(pml4_physical));
    page_tree tree = page_tree_get(pml4_physical, (uint64_t)ptr1);
    log("Inspecting the page table tree for ptr1:", framebuffer);

    snprintf(buf, sizeof(buf), "  Deepest resolved level (Expected 1): %d", tree.min_level_exists);
    log(buf, framebuffer);

    snprintf(buf, sizeof(buf), "  PT entry Present bit (Expected 1): %d", tree.pt.present);
    log(buf, framebuffer);

    snprintf(buf, sizeof(buf), "  Physical PFN mapped: 0x%llx", (unsigned long long)tree.pt.address);
    log(buf, framebuffer);

    if (free_user_pages((uint64_t)ptr4, 1, 0) != 0) {
        log("[CRITICAL] Failed to free user page 4 (ptr4)!", framebuffer);
        return;
    }

    log("--- MEMORY ALLOCATOR TEST COMPLETE ---", framebuffer);
}


void kmain(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    if (memmmap_request.response == NULL
     || memmmap_request.response->entry_count < 1) {
        hcf();
    }

    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];


    log("Detected memory:", framebuffer);
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
        log(buf, framebuffer);
    }
    syscall_init();
    alloc_init(memmmap_request.response);
    test_allocator(framebuffer);
    /*

    log("Detected video modes:", framebuffer);
    for (uint64_t x = 0; x < framebuffer_request.response->framebuffer_count; x++){
        char buf[128];
        snprintf(buf, sizeof(buf), "Screen number: %llu Mode count: %llu", x, framebuffer_request.response->framebuffers[x]->mode_count);
        log(buf, framebuffer);
        for (uint64_t y = 0; y < framebuffer_request.response->framebuffers[x]->mode_count; y++){
            char bufa[128];
            uint64_t w = framebuffer_request.response->framebuffers[x]->modes[y]->width;
            uint64_t h = framebuffer_request.response->framebuffers[x]->modes[y]->height;
            uint64_t bpp = framebuffer_request.response->framebuffers[x]->modes[y]->bpp;
            snprintf(bufa, sizeof(bufa), "%llux%llu@%llu", w, h, bpp);
            log(bufa, framebuffer);
        }
    }
    */

    hcf();
}
