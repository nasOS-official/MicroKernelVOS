#include "cpu.h"
#include <stdint.h>
#include "../../kernel/include/limine_requests.h"
#include "debug.h"
#include "mmu.h"
#include "msr.h"
#include "stdbool.h"

uint32_t cpu_get_count(){
    return 1;
}

void syscall_init();

uintptr_t lapic_base = 0;
uint64_t tsc_ticks_per_ms;
uint64_t tsc_freq;

#define LAPIC_TIMER_REG   (lapic_base + 0x320)
#define LAPIC_INIT_TIMER  (lapic_base + 0x380)
#define LAPIC_CUR_TIMER   (lapic_base + 0x390)
#define LAPIC_DIV_TIMER   (lapic_base + 0x3E0)

bool tsc = false;


uint64_t calibrate_tsc(void) {
    uint16_t pit_ticks = 11931; 

    uint8_t tmp = inb(0x61);
    outb(0x61, (tmp & ~0x02) | 0x01);

    outb(0x43, 0xB0); 
    outb(0x42, (uint8_t)(pit_ticks & 0xFF));
    outb(0x42, (uint8_t)(pit_ticks >> 8));

    uint64_t tsc_start = read_tsc();

    tmp = inb(0x61);
    outb(0x61, tmp | 0x01);

    uint16_t val;
    do {
        outb(0x43, 0x80);
        uint8_t lo = inb(0x42);
        uint8_t hi = inb(0x42);
        val = (hi << 8) | lo;
    } while (val > 0);

    uint64_t tsc_end = read_tsc();

    uint64_t tsc_delta = tsc_end - tsc_start;

    uint64_t tsc_freq = tsc_delta * 100;

    return tsc_freq;
}

uint64_t calibrate_lapic(void) {

    uint16_t pit_ticks = 11931; 

    uint8_t tmp = inb(0x61);
    outb(0x61, (tmp & ~0x02) | 0x01);

    outb(0x43, 0xB0);
    outb(0x42, (uint8_t)(pit_ticks & 0xFF));
    outb(0x42, (uint8_t)(pit_ticks >> 8));

    *(volatile uint32_t*)LAPIC_INIT_TIMER = 0xFFFFFFFF;
    *(volatile uint32_t*)LAPIC_DIV_TIMER = 1;

    tmp = inb(0x61);
    outb(0x61, tmp | 0x01);

    uint16_t val;
    do {
        outb(0x43, 0x80);
        uint8_t lo = inb(0x42);
        uint8_t hi = inb(0x42);
        val = (hi << 8) | lo;
    } while (val > 0);
    
    uint64_t tsc_delta = 0xFFFFFFFF - *(volatile uint32_t*)LAPIC_CUR_TIMER;
    *(volatile uint32_t*)LAPIC_INIT_TIMER = 0;

    uint64_t tsc_freq = tsc_delta * 100;

    return tsc_freq;
}

extern bool map_ki_hardware(uintptr_t table_ptr, uint8_t level, uintptr_t curr_addr, uintptr_t max_addr, uintptr_t *curr_pfn, bool user, bool readonly);


void cpu_init(uint32_t core){
    (void) core;
    syscall_init();

    uint32_t features;
    asm volatile (
        "cpuid"
        : "=d"(features)
        : "a"(0x80000007)
        : "memory"
    );
    uint64_t phys_page = 0xFEE00;
    map_ki_hardware(hhdm_request.response->offset + mmu_get_top_table(), mmu_get_layout()->levels_count - 1, 0xFFFFC00000000000, 0xFFFFC00000000000, &phys_page, false, false);

    lapic_base = 0xFFFFC00000000000;
    *(volatile uint32_t*)LAPIC_TIMER_REG = 0xEF | (1 << 16);
    if (features & (1UL << 8)){
        log_string("[HAL] Using TSC for scheduler");
        uint32_t d, n, f;
        asm volatile (
            "cpuid"
            : "=a"(d), "=b"(n), "=c"(f)
            : "a"(0x15)
            : "memory"
        );

        if (f == 0){
            tsc_freq = calibrate_tsc();
            log_string("TSC CALIBRATION FINISHED\n\r");
        } else {
            tsc_freq = f * (n / d);
            log_string("TSC CALIBRATION NOT NEEDED\n\r");
        }
        tsc_ticks_per_ms = tsc_freq / 100;

     
        uint32_t lvt_value = 0xEF | (2 << 17);
        *(volatile uint32_t*)LAPIC_TIMER_REG = lvt_value;

        uint64_t target_tsc = read_tsc() + tsc_ticks_per_ms;
        wrmsr(IA32_TSC_DEADLINE, target_tsc);
        tsc = true;
    } else {
        log_string("[HAL] Using LAPIC timer for scheduler\n\r");

        uint32_t d, n, f;
        asm volatile (
            "cpuid"
            : "=a"(d), "=b"(n), "=c"(f)
            : "a"(0x15)
            : "memory"
        );

        if (f == 0){
            log_string("[HAL] LAPIC CALIBRATION STARTED\n\r");
            tsc_freq = calibrate_lapic();
            log_string("LAPIC CALIBRATION FINISHED\n\r");
        } else {
            tsc_freq = f * (n / d);
            log_string("LAPIC CALIBRATION NOT NEEDED\n\r");
        }

        *(volatile uint32_t*)LAPIC_DIV_TIMER = 1;

        uint32_t lvt_value = 0xEF;
        *(volatile uint32_t*)LAPIC_TIMER_REG = lvt_value;

        tsc_ticks_per_ms = tsc_freq / 100;
        *(volatile uint32_t*)LAPIC_INIT_TIMER = tsc_ticks_per_ms;


        
    }
    // irq 0xEF
}

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

struct tss_64 {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopm_offset;
} __attribute__((packed));

struct tss_64 tss;

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
extern void sched_entry(void);

void init_idt(void) {
    struct idt_ptr idt_p;

    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, (uint64_t)&exception_entry, 0x8E);
    }
    idt_set_gate(0xEF, (uint64_t)&sched_entry, 0x8E);
    idt_p.limit = (sizeof(struct idt_entry) * 256) - 1;
    idt_p.base  = (uint64_t)&idt;

    asm volatile("lidt %0" : : "m"(idt_p));
}


extern void syscall_entry(void);


struct syscall_frame_x86 {
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

int64_t x86_syscall_handler(struct syscall_frame_x86* regs){
    syscall_frame frame;
    frame.syscall_num = regs->rax;
    frame.arg1 = regs->rdi;
    frame.arg2 = regs->rsi;
    frame.arg3 = regs->rdx;
    frame.arg4 = regs->r10;
    frame.arg5 = regs->r8;
    frame.arg6 = regs->r9;
    return syscall_handler(&frame);
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

uint64_t my_gdt[7] = {
    0x0000000000000000ULL,
    0x00AF9A000000FFFFULL,
    0x00CF92000000FFFFULL,
    0x00AFFB000000FFFFULL,
    0x00CFF2000000FFFFULL,
    0,
    0
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

void set_kernel_stack(void* kernel_SP){
    tss.rsp0 = (uint64_t)kernel_SP;
}

void init_tss() {
    unsigned char* ptr = (unsigned char*)&tss;
    for (unsigned int i = 0; i < sizeof(struct tss_64); i++) {
        ptr[i] = 0;
    }

  
    tss.rsp0 = 0;

    tss.iopm_offset = sizeof(struct tss_64);

    uint64_t base = (uint64_t)&tss;
    uint32_t limit = sizeof(struct tss_64) - 1;

    my_gdt[5] = (limit & 0xFFFF) | 
                ((base & 0xFFFFFF) << 16) | 
                (0x89ULL << 40) |                  
                (((uint64_t)limit & 0xF0000) << 32) | 
                (((base & 0xFF000000ULL) >> 24) << 56);

    my_gdt[6] = base >> 32;

    struct gdt_ptr gdt_p;
    gdt_p.limit = sizeof(my_gdt) - 1;
    gdt_p.base  = (uint64_t)&my_gdt;

    asm volatile (
        "lgdt %0\n\t"
        :
        : "m"(gdt_p)
        : "memory"
    );

    // asm volatile (
    //     "lgdt %0\n\t"
    //     "push $0x08\n\t"
    //     "lea 1f(%%rip), %%rax\n\t"
    //     "push %%rax\n\t"
    //     "lretq\n\t"
    //     "1:\n\t"
        
    //     "mov $0x10, %%ax\n\t"
    //     "mov %%ax, %%ds\n\t"
    //     "mov %%ax, %%es\n\t"
    //     "mov %%ax, %%ss\n\t"
    //     :
    //     : "m"(gdt_p)
    //     : "rax", "memory"
    // );

    uint16_t tss_selector = 0x28;
    asm volatile ("ltr %0" : : "r"(tss_selector));
}

void syscall_init(){
    init_idt();
    init_tss();
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



void set_thread_info_ptr(struct task_control_block_t* ptr){
    wrmsr(IA32_GS_BASE, (uint64_t)ptr);
    wrmsr(IA32_KERNEL_GS_BASE, (uint64_t)ptr);
}

struct task_control_block_t* get_thread_info_ptr(){
    return (struct task_control_block_t*)rdmsr(IA32_GS_BASE);
}


extern void sched_handler();

void x86_sched_handler(){
    sched_handler();


    if (tsc){
        uint64_t next_deadline = read_tsc() + tsc_ticks_per_ms;
        wrmsr(IA32_TSC_DEADLINE, next_deadline);
    } else {
        *(volatile uint32_t*)LAPIC_INIT_TIMER = tsc_ticks_per_ms;
    }


    *(volatile uint32_t*)(lapic_base + 0xB0) = 0;
}