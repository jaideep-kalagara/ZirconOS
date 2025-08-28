#pragma once

#include <stdint.h>

extern uint32_t initial_page_dir[1024];

void i686_init_memory(uint32_t mem_high, uint32_t physical_alloc_start);
void pmm_init(uint32_t mem_low, uint32_t mem_high);
void dump_physical_memory_bitmap();
uint32_t pmm_alloc_page_frame();
uint32_t *mem_get_current_page_dir();
void mem_change_page_dir(uint32_t *pd);
void sync_page_dirs();
void mem_map_page(uint32_t vaddr, uint32_t paddr, uint32_t flags);

#define KERNEL_START 0xC0000000
#define KERNEL_MALLOC 0xD0000000
#define REC_PAGEDIR ((uint32_t *)0xFFFFF000)
#define REC_PAGETABLE(i) ((uint32_t *)(0xFFC00000 + ((i) << 12)))
#define PAGE_FLAG_PRESENT (1 << 0)
#define PAGE_FLAG_WRITE (1 << 1)
#define PAGE_FLAG_OWNER (1 << 9)