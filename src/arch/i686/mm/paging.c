#include "arch/i686/mm/paging.h"

#include <stdint.h>
#include <string.h>
#include <util/ceil.h>

static uint32_t page_frame_min;
static uint32_t page_frame_max;
static uint32_t total_alloc;

uint8_t physical_memory_bitmap[NUM_PAGE_FRAMES /
                               8]; // TODO: do dynamically, bit array

void pmm_init(uint32_t mem_low, uint32_t mem_high) {
  page_frame_min = CEIL_DIV(mem_low, 0x1000);
  page_frame_max = mem_high / 0x1000;
  total_alloc = 0;

  memset(physical_memory_bitmap, 0, sizeof(physical_memory_bitmap));
}