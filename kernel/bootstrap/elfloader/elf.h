// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __ELF_H
#define __ELF_H

#include <stdint.h>

#define PT_LOAD 0x00000001
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

typedef struct {
    uint8_t EI_MAG[4];
    uint8_t EI_CLASS;
    uint8_t EI_DATA;
    uint8_t EI_VERSION;
    uint8_t EI_OSABI;
    uint8_t EI_ABIVERSION;
    uint8_t EI_PAD[7];
} e_ident_t;

typedef struct {
    e_ident_t e_ident;
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf_format;


typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} p_header;

typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} s_header;

void* loadelf(const uint8_t* addr, uint64_t lenght);

#endif