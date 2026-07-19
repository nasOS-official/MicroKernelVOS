// SPDX-License-Identifier: GPL-2.0-or-later
#include "paging.h"
#include "tasks.h"
#include "msr.h"
#include <stdint.h>
#include <syscall.h>
#include <abstractions.h>

struct idt_entry {
    uint16_t offset_low;    
    uint16_t selector;
    uint8_t  ist;
    uint8_t  types_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;   
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct idt_entry idt[256];

void idt_set_gate(uint8_t vector, uint64_t handler, uint8_t flags) {
    idt[vector].offset_low  = (uint16_t)(handler & 0xFFFF);
    idt[vector].selector    = 0x08;
    idt[vector].ist         = 0;
    idt[vector].types_attr  = flags;
    idt[vector].offset_mid  = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[vector].offset_high = (uint32_t)((handler >> 32) & 0xFFFFFFFF);
    idt[vector].reserved    = 0;
}

extern void exception_entry(void); 

void init_idt(void) {
    struct idt_ptr idt_p;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, (uint64_t)&exception_entry, 0x8E);
    }

    idt_p.limit = (sizeof(struct idt_entry) * 256) - 1;
    idt_p.base  = (uint64_t)&idt;

    asm volatile("lidt %0" : : "m"(idt_p));
}


extern void syscall_entry(void);

struct syscall_frame {
    uint64_t nothing;
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
};

int64_t syscall_handler(struct syscall_frame* regs){
    uint64_t syscall_num = regs->rax;
    uint64_t arg1 = regs->rdi;
    uint64_t arg2 = regs->rsi;
    uint64_t arg3 = regs->rdx;
    uint64_t arg4 = regs->r10;
    uint64_t arg5 = regs->r8;
    uint64_t arg6 = regs->r9;
    (void)arg6;
    switch (syscall_num) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            return -1;
            break;
        case 7:
            return map_hardware_user(arg1,  arg2, arg3, arg4, arg5);
            break;
        case 8:
            return (int64_t)alloc_user_pages(arg1, arg2, arg3);
        case 9:
            return free_user_pages(arg1, arg2, arg3);
        default:
            return -1;
            break;
    }
}

void wrmsr(uint32_t addr, uint64_t data){
    uint32_t eax = (uint64_t)(data) & 0xFFFFFFFF;
    uint32_t edx = (uint64_t)(data) >> 32;
    asm volatile(
        "wrmsr"
        :
        : "c"(addr), "a"(eax), "d"(edx)
        : "memory");
}

uint64_t rdmsr(uint32_t addr){
    uint32_t eax;
    uint32_t edx;
    asm volatile(
        "rdmsr"
        : "=a"(eax), "=d"(edx)
        : "c"(addr)
        : "memory");
    return ((uint64_t)edx << 32) | (uint64_t)eax;
}

#define EFER_SCE  (1 << 0)


struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

uint64_t my_gdt[5] = {
    0x0000000000000000ULL,
    0x00AF9A000000FFFFULL,
    0x00CF92000000FFFFULL,
    0x00AFFB000000FFFFULL,
    0x00CFF2000000FFFFULL
};

void init_gdt(void) {
    struct gdt_ptr gdt_p;
    gdt_p.limit = sizeof(my_gdt) - 1;
    gdt_p.base  = (uint64_t)&my_gdt;

    asm volatile (
        "lgdt %0\n\t"
        "push $0x08\n\t"
        "lea 1f(%%rip), %%rax\n\t"
        "push %%rax\n\t"
        "lretq\n\t"
        "1:\n\t"
        
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%ss\n\t"
        :
        : "m"(gdt_p)
        : "rax", "memory"
    );
}


void syscall_init(){
    init_idt();
    init_gdt();
    uint64_t efer = rdmsr(IA32_EFER);
    efer |= EFER_SCE;
    wrmsr(IA32_EFER, efer);
    wrmsr(IA32_FMASK, 0x200);
    wrmsr(IA32_LSTAR, (uint64_t)&syscall_entry);

    uint64_t star = 0;
    star |= ((uint64_t)0x08) << 32;
    star |= ((uint64_t)0x18) << 48;
    
    wrmsr(IA32_STAR, star);

    wrmsr(IA32_FMASK, 0x200); 
}


void set_thread_info_ptr(task_control_block_t* ptr){
    wrmsr(IA32_GS_BASE, (uint64_t)ptr);
    wrmsr(IA32_KERNEL_GS_BASE, (uint64_t)ptr);
}

task_control_block_t* get_thread_info_ptr(){
    return (task_control_block_t*)rdmsr(IA32_GS_BASE);
}

