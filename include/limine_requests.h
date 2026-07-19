// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __LIMINE_REQUESTS_H
#define __LIMINE_REQUESTS_H
#include <stdint.h>
#include <limine.h>

extern volatile uint64_t limine_base_revision[];

extern volatile struct limine_framebuffer_request framebuffer_request;

extern volatile struct limine_memmap_request memmmap_request;

extern volatile struct limine_stack_size_request stack_request;

extern volatile struct limine_hhdm_request hhdm_request;

extern volatile struct limine_module_request module_request;

#endif
