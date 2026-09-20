#ifndef __RISCV64_MMU_H
#define __RISCV64_MMU_H

#include <stdint.h>

typedef struct rv64_mmu_entry{
    uint64_t valid        : 1;
    uint64_t readable     : 1;
    uint64_t writable     : 1;
    uint64_t xecute       : 1;
    uint64_t user         : 1;
    uint64_t global       : 1;
    uint64_t accessed     : 1;
    uint64_t dirty        : 1;
    uint64_t rsw          : 2;
    uint64_t address      : 44;
    uint64_t reserved     : 10;
} rv64_mmu_entry;

#endif