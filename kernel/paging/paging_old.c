

// int64_t free_ki_pages(uint64_t user_vaddr, uint64_t count, bool user, uint64_t flags){
//     uint64_t pml4_physical = mmu_get_top_table();
    
//     if (count == 0 || allocated_pages < count){
//         return -1;
//     }
//     if ((user && user_vaddr + (count * 0x1000) >= 0x0000800000000000) || (!user && user_vaddr < 0x0000800000000000)) {
//         return -1;
//     }


//     if (!(flags & UNSAFE_FLAG)){
//         if (!scan_pages(user_vaddr, count, false)){
//             return -1;
//         }

//     }


//     uint64_t start_addr = user_vaddr;
//     uint64_t end_addr = start_addr + ((count - 1) * 0x1000);
//     uint64_t hhdm_offset = hhdm_request.response->offset;

//     page_table_entry* pml4_table = (page_table_entry*)(hhdm_offset + pml4_physical);

//     for (uint64_t a = PML4_INDEX(start_addr); a < PML4_INDEX(end_addr) + 1; a++) {
//         if (!pml4_table[a].present) continue;
//         page_table_entry* pdpt_table = (page_table_entry*)(hhdm_offset + (pml4_table[a].address << 12));

//         for (uint64_t b = (a == PML4_INDEX(start_addr) ? PDPT_INDEX(start_addr) : 0); b < (a == PML4_INDEX(end_addr) ? PDPT_INDEX(end_addr) + 1 : 512); b++) {
//             if (!pdpt_table[b].present) continue;

//             page_table_entry* pd_table = (page_table_entry*)(hhdm_offset + (pdpt_table[b].address << 12));

//             for (uint64_t c = (a == PML4_INDEX(start_addr) && b == PDPT_INDEX(start_addr) ? PD_INDEX(start_addr) : 0); c < (a == PML4_INDEX(end_addr) && b == PDPT_INDEX(end_addr) ? PD_INDEX(end_addr) + 1 : 512); c++) {
//                 if (!pd_table[c].present) continue;
//                 page_table_entry* pt_table = (page_table_entry*)(hhdm_offset + (pd_table[c].address << 12));
                
//                 uint64_t d_start = (a == PML4_INDEX(start_addr) && b == PDPT_INDEX(start_addr) && c == PD_INDEX(start_addr) ? PT_INDEX(start_addr) : 0);
//                 uint64_t d_end = (a == PML4_INDEX(end_addr) && b == PDPT_INDEX(end_addr) && c == PD_INDEX(end_addr) ? PT_INDEX(end_addr) + 1 : 512);

//                 for (uint64_t d = d_start; d < d_end; d++) {
//                     if (pt_table[d].present) { //todo: change to flags check when swap will be realized

//                         uint64_t page = pt_table[d].address;
//                         if (pages_map[page].flags & PAGE_FREE){
//                             panic();
//                         }
//                         //todo: remove free mem tables if entire region is free
//                         pt_table[d].present = 0;
//                         pt_table[d].address = 0;                        
//                         mmu_update_addr(VIRT_ADDR(a, b, c, d));

//                         if (!(pages_map[page].flags & PAGE_HARDWARE)){
//                             pages_map[page]._allocated.counters--;

//                             if (pages_map[page]._allocated.counters == 0){
//                                 free_buddy_pages(&pages_map[page], 0);
//                             }
//                         }


//                     }
//                 }
//             }
//         }
//     }
//     return 0;
// }


// void* alloc_ki_pages(uint64_t user_vaddr, uint64_t count, bool user, uint64_t flags) {
//     uint64_t pml4_physical = mmu_get_top_table();

//     if (count == 0 || allocated_pages + count > total_pages){
//         return NULL;
//     }
//     if ((user && user_vaddr + ((count - 1) * 0x1000) >= 0x0000800000000000) || (!user && user_vaddr < 0x0000800000000000)) {
//         return NULL;
//     }

//     if (!(flags & UNSAFE_FLAG)){
//         if (!scan_pages(user_vaddr, count, true)){
//             return NULL;
//         }

//     }


//     uint64_t start_addr = user_vaddr;
//     uint64_t end_addr = start_addr + ((count - 1) * 0x1000);
//     uint64_t hhdm_offset = hhdm_request.response->offset;

//     page_table_entry* pml4_table = (page_table_entry*)(hhdm_offset + pml4_physical);

//     for (uint64_t a = PML4_INDEX(start_addr); a < PML4_INDEX(end_addr) + 1; a++) {
//         if (!pml4_table[a].present) {
//             struct Page* new_table_page = alloc_pages(0);
//             if (!new_table_page) panic();

//             uint64_t new_table_phys = (new_table_page - pages_map) * 4096;
//             clear_page_table((void*)(hhdm_offset + new_table_phys));

//             pml4_table[a].present = 1;
//             pml4_table[a].writable = 1;
//             pml4_table[a].user = user;
//             pml4_table[a].address = new_table_phys >> 12;
//         }
//         page_table_entry* pdpt_table = (page_table_entry*)(hhdm_offset + (pml4_table[a].address << 12));

//         for (uint64_t b = (a == PML4_INDEX(start_addr) ? PDPT_INDEX(start_addr) : 0); b < (a == PML4_INDEX(end_addr) ? PDPT_INDEX(end_addr) + 1 : 512); b++) {
//             if (!pdpt_table[b].present) {
//                 struct Page* new_table_page = alloc_pages(0);
//                 if (!new_table_page) panic();

//                 uint64_t new_table_phys = (new_table_page - pages_map) * 4096;
//                 clear_page_table((void*)(hhdm_offset + new_table_phys));

//                 pdpt_table[b].present = 1;
//                 pdpt_table[b].writable = 1;
//                 pdpt_table[b].user = user;
//                 pdpt_table[b].address = new_table_phys >> 12;
//             }
//             page_table_entry* pd_table = (page_table_entry*)(hhdm_offset + (pdpt_table[b].address << 12));

//             for (uint64_t c = (a == PML4_INDEX(start_addr) && b == PDPT_INDEX(start_addr) ? PD_INDEX(start_addr) : 0); c < (a == PML4_INDEX(end_addr) && b == PDPT_INDEX(end_addr) ? PD_INDEX(end_addr) + 1 : 512); c++) {
//                 if (!pd_table[c].present) {
//                     struct Page* new_table_page = alloc_pages(0);
//                     if (!new_table_page) panic();

//                     uint64_t new_table_phys = (new_table_page - pages_map) * 4096;
//                     clear_page_table((void*)(hhdm_offset + new_table_phys));

//                     pd_table[c].present = 1;
//                     pd_table[c].writable = 1;
//                     pd_table[c].user = user;
//                     pd_table[c].address = new_table_phys >> 12;
//                 }
//                 page_table_entry* pt_table = (page_table_entry*)(hhdm_offset + (pd_table[c].address << 12));
                
//                 uint64_t d_start = (a == PML4_INDEX(start_addr) && b == PDPT_INDEX(start_addr) && c == PD_INDEX(start_addr) ? PT_INDEX(start_addr) : 0);
//                 uint64_t d_end = (a == PML4_INDEX(end_addr) && b == PDPT_INDEX(end_addr) && c == PD_INDEX(end_addr) ? PT_INDEX(end_addr) + 1 : 512);

//                 for (uint64_t d = d_start; d < d_end; d++) {
//                     if (!pt_table[d].present) {
//                         struct Page* phys_page = alloc_pages(0);
//                         if (!phys_page) panic();

//                         uint64_t paddr = (phys_page - pages_map);
                        
//                         // pt_table[d].address = paddr;
//                         // pt_table[d].user = user;
//                         // pt_table[d].writable = 1;
//                         // pt_table[d].present = 1;
//                         page_table_entry1 page = {
//                             .address = paddr,
//                             .user = user,
//                             .writable = 1,
//                             .present = 1,
//                             .no_execute = 0,
//                             .nocache = 0,
//                             .writethrough = 0,
//                             .paged = 0,
//                             .lazyalloc = 0,
//                             .hardware = 0,
//                             .dirty = 0
//                         };
//                         mmu_set_elem_info((uintptr_t)pt_table, VIRT_ADDR(a, b, c, d), 0, &page);

//                         mmu_update_addr(VIRT_ADDR(a, b, c, d));
//                     }
//                 }
//             }
//         }
//     }
    

//     return (void*)user_vaddr;
// }


/*
// void* map_ki_hardware(uint64_t user_vaddr,	uint64_t count, uint64_t phys_page, uint64_t mem_type, uint64_t flags, bool user){
//     uint64_t pml4_physical = mmu_get_top_table();

//     task_control_block_t* tcb = get_thread_info_ptr();
//     if (!(tcb->process->rights & PROC_DRIVER_RIGHT)){
//         return NULL;
//     }

//     switch (mem_type) {
//         case LIMINE_MEMMAP_RESERVED:
//             if (!(tcb->process->rights & PROC_MEM_RESERVED_RIGHT))
//                 return NULL;
//             break;
//         case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
//             if (!(tcb->process->rights & PROC_MEM_ACPI_TABLE_RIGHT))
//                 return NULL;
//             break;
//         case LIMINE_MEMMAP_ACPI_NVS:
//             if (!(tcb->process->rights & PROC_MEM_ACPI_NVS_RIGHT))
//                 return NULL;
//             break;
//         case LIMINE_MEMMAP_FRAMEBUFFER:
//             if (!(tcb->process->rights & PROC_MEM_FRAMEBUFFER_RIGHT))
//                 return NULL;
//             break;
//         case LIMINE_MEMMAP_RESERVED_MAPPED:
//             if (!(tcb->process->rights & PROC_MEM_RESERVER_MAPPED_RIGHT))
//                 return NULL;
//             break;
//         default:
//             return 0;
//     }



//     bool readonly = false;
//     if (mem_type == LIMINE_MEMMAP_ACPI_RECLAIMABLE || flags & READONLY_FLAG){
//         readonly = true;
//     }

    
//     if ((user && user_vaddr + (count * 0x1000) >= 0x0000800000000000) || (!user && user_vaddr < 0x0000800000000000)) {
//         return NULL;
//     }

//     if (!scan_pages(user_vaddr, count, true)){
//         return NULL;
//     }


//     uint64_t start_sel = 0;
//     bool found = false;
//     for (uint64_t x = 0; x < memmmap_request.response->entry_count; x++){
//         if (memmmap_request.response->entries[x]->base >> 12 <= phys_page && (memmmap_request.response->entries[x]->base + memmmap_request.response->entries[x]->length) >> 12 >= phys_page + count){
//             start_sel = x;
//             found = true;
//             break;
//         }
//     }

//     if (memmmap_request.response->entries[start_sel]->type != mem_type || !found){
//         return NULL;
//     }


//     uint64_t start_addr = user_vaddr;
//     uint64_t end_addr = start_addr + ((count - 1) * 0x1000);
//     uint64_t hhdm_offset = hhdm_request.response->offset;

//     page_table_entry* pml4_table = (page_table_entry*)(hhdm_offset + pml4_physical);
//     uint64_t curr_pfn = phys_page;

//     for (uint64_t a = PML4_INDEX(start_addr); a < PML4_INDEX(end_addr) + 1; a++) {
//         if (!pml4_table[a].present) {
//             struct Page* new_table_page = alloc_pages(0);
//             if (!new_table_page) panic();

//             uint64_t new_table_phys = (new_table_page - pages_map) * 4096;
//             clear_page_table((void*)(hhdm_offset + new_table_phys));

//             pml4_table[a].present = 1;
//             pml4_table[a].writable = 1;
//             pml4_table[a].user = user;
//             pml4_table[a].address = new_table_phys >> 12;
//         }
//         page_table_entry* pdpt_table = (page_table_entry*)(hhdm_offset + (pml4_table[a].address << 12));

//         for (uint64_t b = (a == PML4_INDEX(start_addr) ? PDPT_INDEX(start_addr) : 0); b < (a == PML4_INDEX(end_addr) ? PDPT_INDEX(end_addr) + 1 : 512); b++) {
//             if (!pdpt_table[b].present) {
//                 struct Page* new_table_page = alloc_pages(0);
//                 if (!new_table_page) panic();

//                 uint64_t new_table_phys = (new_table_page - pages_map) * 4096;
//                 clear_page_table((void*)(hhdm_offset + new_table_phys));

//                 pdpt_table[b].present = 1;
//                 pdpt_table[b].writable = 1;
//                 pdpt_table[b].user = user;
//                 pdpt_table[b].address = new_table_phys >> 12;
//             }
//             page_table_entry* pd_table = (page_table_entry*)(hhdm_offset + (pdpt_table[b].address << 12));

//             for (uint64_t c = (a == PML4_INDEX(start_addr) && b == PDPT_INDEX(start_addr) ? PD_INDEX(start_addr) : 0); c < (a == PML4_INDEX(end_addr) && b == PDPT_INDEX(end_addr) ? PD_INDEX(end_addr) + 1 : 512); c++) {
//                 if (!pd_table[c].present) {
//                     struct Page* new_table_page = alloc_pages(0);
//                     if (!new_table_page) panic();

//                     uint64_t new_table_phys = (new_table_page - pages_map) * 4096;
//                     clear_page_table((void*)(hhdm_offset + new_table_phys));

//                     pd_table[c].present = 1;
//                     pd_table[c].writable = 1;
//                     pd_table[c].user = user;
//                     pd_table[c].address = new_table_phys >> 12;
//                 }
//                 page_table_entry* pt_table = (page_table_entry*)(hhdm_offset + (pd_table[c].address << 12));
                
//                 uint64_t d_start = (a == PML4_INDEX(start_addr) && b == PDPT_INDEX(start_addr) && c == PD_INDEX(start_addr) ? PT_INDEX(start_addr) : 0);
//                 uint64_t d_end = (a == PML4_INDEX(end_addr) && b == PDPT_INDEX(end_addr) && c == PD_INDEX(end_addr) ? PT_INDEX(end_addr) + 1 : 512);

//                 for (uint64_t d = d_start; d < d_end; d++) {
//                     if (!pt_table[d].present) {
   
                        
//                         pt_table[d].address = curr_pfn;
//                         pt_table[d].user = user;
//                         pt_table[d].writable = !readonly;
//                         pt_table[d].present = 1;

//                         mmu_update_addr(VIRT_ADDR(a, b, c, d));
//                     }
//                     curr_pfn++;
//                 }
//             }
//         }
//     }
//     return (void*)user_vaddr;
// }
*/