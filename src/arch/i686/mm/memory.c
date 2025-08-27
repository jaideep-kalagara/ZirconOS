#include "arch/i686/mm/memory.h"
#include "arch/i686/mm/paging.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void invalidate(uint32_t vaddr) { asm volatile("invlpg %0" ::"m"(vaddr)); }

void i686_init_memory(uint32_t mem_high, uint32_t physical_alloc_start) {
  initial_page_dir[0] = 0;
  invalidate(0);
  initial_page_dir[1023] = ((uint32_t)initial_page_dir - KERNEL_START) |
                           PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;

  invalidate(0xFFFFF000);
  pmm_init(physical_alloc_start, mem_high);      // setup pmm
  memset(page_dirs, 0, 0x1000 * NUM_PAGES_DIRS); // set to zero
  memset(page_dir_used, 0, NUM_PAGES_DIRS);      // set to zero
  printf("Memory initialized.\n");
}
