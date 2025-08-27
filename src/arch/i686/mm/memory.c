#include "arch/i686/mm/memory.h"
#include "arch/i686/mm/paging.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void i686_init_memory(uint32_t mem_high, uint32_t physical_alloc_start) {
  pmm_init(physical_alloc_start, mem_high);

  uint32_t pd_pa = ((uint32_t)initial_page_dir - KERNEL_START) & ~0xFFFu;

  // place self-ref and activate PD/PT windows
  ((volatile uint32_t *)initial_page_dir)[PD_SELF_INDEX] = pd_pa | PG_P | PG_RW;
  wrcr3(pd_pa); // flush & enable PD/PT window at 0xFFFFF000 / 0xFFC00000

  printf("Memory initialized.\n");
}
