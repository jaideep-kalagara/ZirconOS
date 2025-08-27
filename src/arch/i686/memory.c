#include "arch/i686/memory.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <util/ceil.h>

static uint32_t page_frame_min;
static uint32_t page_frame_max;
static uint32_t total_alloc;

#define NUM_PAGES_DIRS 256
#define NUM_PAGE_FRAMES (0x100000000 / 0x1000 / 8)

uint8_t physical_memory_bitmap[NUM_PAGE_FRAMES / 8];  // TODO: Dynamically, bit array

static uint32_t page_dirs[NUM_PAGES_DIRS][1024] __attribute__((aligned(4096)));
static uint8_t page_dir_used[NUM_PAGES_DIRS];

void invalidate(uint32_t vaddr) {
    asm volatile("invlpg %0" ::"m"(vaddr));
}

void pmm_init(uint32_t mem_low, uint32_t mem_high) {
    page_frame_min = CEIL_DIV(mem_low, 0x1000);
    page_frame_max = mem_high / 0x1000;
    total_alloc = 0;

    memset(physical_memory_bitmap, 0, sizeof(physical_memory_bitmap));
}

void i686_init_memory(uint32_t mem_high, uint32_t physical_alloc_start) {
    initial_page_dir[0] = 0;
    invalidate(0);
    initial_page_dir[1023] = ((uint32_t)initial_page_dir - KERNEL_START) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
    invalidate(0xFFFFF000);

    pmm_init(physical_alloc_start, mem_high);
    memset(page_dirs, 0, 0x1000 * NUM_PAGES_DIRS);
    memset(page_dir_used, 0, NUM_PAGES_DIRS);

    printf("Memory initialized.\n");
}
