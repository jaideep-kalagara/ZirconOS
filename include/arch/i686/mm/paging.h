#pragma once

#include <stdint.h>

#define PAGE_FLAG_PRESENT (1 << 0)
#define PAGE_FLAG_WRITE (1 << 1)

#define NUM_PAGES_DIRS 256
#define NUM_PAGE_FRAMES (0x100000000 / 0x1000 / 8)

static uint32_t page_dirs[NUM_PAGES_DIRS][1024] __attribute__((aligned(4096)));
static uint32_t page_dir_used[NUM_PAGES_DIRS];

void pmm_init(uint32_t mem_low, uint32_t mem_high);