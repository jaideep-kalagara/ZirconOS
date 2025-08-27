#pragma once

#include <stdint.h>

extern uint32_t initial_page_dir[1024];
#define KERNEL_START 0xC0000000

void i686_init_memory(uint32_t mem_high, uint32_t physical_alloc_start);