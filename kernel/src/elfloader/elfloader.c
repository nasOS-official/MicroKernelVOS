// SPDX-License-Identifier: GPL-2.0-or-later
#include <stdint.h>
#include <string.h>
#include "debug.h"
#include "elf.h"
#include "mmu.h"
#include "paging.h"
#include "tasks.h"

void* loadelf(process_t* process, const uint8_t* addr, uint64_t lenght){
	// uint8_t* addri = addr;
    (void) lenght; // add checks for safety;
	elf_format *elf_header = (void*)addr;
	void* entry_point = (void*)elf_header->e_entry;

	uintptr_t top_table = mmu_get_top_table();
	mmu_set_top_table(process->cr3);
	for (int x = 0; x < elf_header->e_phnum; x++){
		p_header *program_header = (void*)(addr + elf_header->e_phoff + (elf_header->e_phentsize * x));
		if (program_header->p_type == PT_LOAD){
			const uint8_t* ptr = addr + program_header->p_offset;
            alloc_user_pages(process->cr3, program_header->p_vaddr, ((program_header->p_vaddr & 0xFFF) + program_header->p_memsz + 0xFFF) >> 12, UNSAFE_FLAG);
			if (program_header->p_flags & PF_W){
				memset((void*)program_header->p_vaddr, 0, program_header->p_memsz);
			}
            memcpy((void*)program_header->p_vaddr, ptr, program_header->p_filesz);
		}

	}
	mmu_set_top_table(top_table);
    return entry_point;
}