#pragma once

#include <stdint.h>

extern uint32_t initial_page_dir[1024];

void i686_init_memory(uint32_t mem_high, uint32_t physical_alloc_start);
void pmm_init(uint32_t mem_low, uint32_t mem_high);

#define KERNEL_START 0xC0000000
#define PAGE_FLAG_PRESENT (1 << 0)
#define PAGE_FLAG_WRITE (1 << 1)