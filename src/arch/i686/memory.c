#include "arch/i686/memory.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <util/ceil.h>

static uint32_t page_frame_min;
static uint32_t page_frame_max;
static uint32_t total_alloc;

#define NUM_PAGES_DIRS 256                         // TODO: Dynamically
#define NUM_PAGE_FRAMES (0x100000000 / 0x1000 / 8) // 128 KiB

uint8_t physical_memory_bitmap[NUM_PAGE_FRAMES]; // TODO: Dynamically, bit array

static uint32_t page_dirs[NUM_PAGES_DIRS][1024] __attribute__((aligned(4096)));
static uint8_t page_dir_used[NUM_PAGES_DIRS];
static int mem_num_vpages;

void invalidate(uint32_t vaddr) { asm volatile("invlpg %0" ::"m"(vaddr)); }

void pmm_init(uint32_t mem_low, uint32_t mem_high) {
  page_frame_min = CEIL_DIV(mem_low, 4096);
  page_frame_max = mem_high / 0x1000;

  // Start with all frames used
  memset(physical_memory_bitmap, 0xFF, sizeof physical_memory_bitmap);

  // Mark only [min, max) as free
  for (uint32_t bit = page_frame_min; bit < page_frame_max; ++bit) {
    physical_memory_bitmap[bit >> 3] &= (uint8_t)~(1u << (bit & 7u));
  }

  total_alloc = 0;
}

uint32_t pmm_alloc_page_frame(void) {
  printf("page_frame_min: %d\n", page_frame_min);
  printf("page_frame_max: %d\n", page_frame_max);
  for (uint32_t f = page_frame_min; f < page_frame_max; ++f) {
    uint32_t byte = f >> 3;
    uint8_t mask = (uint8_t)(1u << (f & 7u));
    printf("%d %d\n", byte, mask);
    if ((physical_memory_bitmap[byte] & mask) == 0) { // free
      physical_memory_bitmap[byte] |= mask;           // mark used
      total_alloc++;
      return f << 12; // phys addr
    }
  }
  return 0; // out of frames
}
uint32_t *mem_get_current_page_dir() {
  uint32_t pd;

  asm volatile("mov %%cr3, %0" : "=r"(pd));
  pd += KERNEL_START;
  return (uint32_t *)pd;
}

void mem_change_page_dir(uint32_t *pd) {
  pd = (uint32_t *)(((uint32_t)pd) - KERNEL_START);

  asm volatile("mov %0, %%eax \n mov %%eax, %%cr3 \n" ::"m"(pd));
}

void sync_page_dirs() {
  for (uint32_t i = 0; i < NUM_PAGES_DIRS; i++) {
    if (page_dir_used[i]) {
      uint32_t *page_dir = page_dirs[i];

      for (int i = 768; i < 1023; i++) {
        page_dir[i] = initial_page_dir[i] & ~PAGE_FLAG_OWNER;
      }
    }
  }
}

void mem_map_page(uint32_t vaddr, uint32_t paddr, uint32_t flags) {
  uint32_t *prev_page_dir = 0;

  if (vaddr >= KERNEL_START) {
    prev_page_dir = mem_get_current_page_dir();
    if (prev_page_dir != initial_page_dir) {
      mem_change_page_dir(initial_page_dir);
    }
  }

  uint32_t pd_index = vaddr >> 22;
  uint32_t pt_index = vaddr >> 12 & 0x3FF;

  uint32_t *page_dir = REC_PAGEDIR;
  uint32_t *page_table = REC_PAGETABLE(pd_index);

  if (!(page_dir[pd_index] & PAGE_FLAG_PRESENT)) {
    uint32_t pt_paddr = pmm_alloc_page_frame();
    page_dir[pd_index] = pt_paddr | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE |
                         PAGE_FLAG_OWNER | flags;
    invalidate(vaddr);

    for (uint32_t i = 0; i < 1024; i++) {
      page_table[i] = 0;
    }
  }

  page_table[pt_index] = paddr | PAGE_FLAG_PRESENT | flags;
  mem_num_vpages++;
  invalidate(vaddr);

  if (prev_page_dir != 0) {
    sync_page_dirs();
    if (prev_page_dir != initial_page_dir) {
      mem_change_page_dir(prev_page_dir);
    }
  }
}

void dump_physical_memory_bitmap() {
  printf("Physical memory bitmap:\n");

  for (uint32_t i = 0; i < NUM_PAGE_FRAMES / 8; i++) {
    printf("0x%X ", physical_memory_bitmap[i]);
  }
  printf("\n");
}

void i686_init_memory(uint32_t mem_high, uint32_t physical_alloc_start) {
  mem_num_vpages = 0;
  initial_page_dir[0] = 0;
  invalidate(0);
  initial_page_dir[1023] = ((uint32_t)initial_page_dir - KERNEL_START) |
                           PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
  invalidate(0xFFFFF000);

  pmm_init(physical_alloc_start, mem_high);
  memset(page_dirs, 0, 0x1000 * NUM_PAGES_DIRS);
  memset(page_dir_used, 0, NUM_PAGES_DIRS);

  printf("Memory initialized.\n");
}
