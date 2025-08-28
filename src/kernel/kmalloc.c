#include "kernel/kmalloc.h"
#include <arch/i686/memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000u
#endif

// ---------- alignment helpers ----------
static inline uint32_t align_up(uint32_t x, uint32_t a) {
  return (x + (a - 1)) & ~(a - 1);
}
#define ALIGN 8u

// ---------- heap state ----------
static uint32_t heap_base;    // KERNEL_MALLOC
static uint32_t mapped_bytes; // bytes actually mapped
static uint32_t brk_offset;   // bump pointer offset
static bool initialized;

// very small free list node (header lives before user ptr)
typedef struct block {
  uint32_t size;
  struct block *next;
} block_t;

static block_t *free_list = NULL;

static inline uint32_t header_size(void) {
  return align_up((uint32_t)sizeof(block_t), ALIGN);
}

// ---------- mapping ----------
static bool map_more_until(uint32_t want_bytes) {
  // Map one page at a time and advance mapped_bytes ONLY on success.
  while (mapped_bytes < want_bytes) {
    uint32_t phys = pmm_alloc_page_frame();
    if (!phys) {
      // out of physical frames; do not over-report mapped_bytes
      return false;
    }
    // map next page at heap_base + mapped_bytes (page-aligned)
    uint32_t va = heap_base + mapped_bytes;
    mem_map_page(va, phys, PAGE_FLAG_WRITE);

    // Optional sanity: write one byte so a bad mapping faults right here
    *(volatile uint8_t *)va = *(volatile uint8_t *)va;

    mapped_bytes += PAGE_SIZE;
  }
  return true;
}

static bool ensure_capacity(uint32_t end_bytes) {
  // round target up to page boundary so we don’t remap the same page repeatedly
  uint32_t need = align_up(end_bytes, PAGE_SIZE);
  if (need <= mapped_bytes)
    return true;
  return map_more_until(need);
}

// ---------- public API ----------
void kmalloc_init(uint32_t initial_heap_size) {
  heap_base = KERNEL_MALLOC;
  mapped_bytes = 0;
  brk_offset = 0;
  free_list = NULL;
  initialized = true;

  // pre-map some space (rounded up)
  if (!ensure_capacity(initial_heap_size)) {
    printf("kmalloc_init: failed to map initial heap (%u bytes)\n",
           initial_heap_size);
  }
}

void *kmalloc(uint32_t size) {
  if (!initialized || size == 0)
    return NULL;

  const uint32_t need = align_up(size, ALIGN);
  const uint32_t hdr = header_size();

  // 1) try free list (first-fit, no split)
  block_t **prev = &free_list;
  for (block_t *b = free_list; b; b = b->next) {
    if (b->size >= need) {
      *prev = b->next;
      return (void *)((uint8_t *)b + hdr);
    }
    prev = &b->next;
  }

  // 2) bump allocation
  uint32_t start = align_up(brk_offset, ALIGN);
  uint32_t end = start + hdr + need;

  if (!ensure_capacity(end)) {
    // out of frames → no write into unmapped memory
    return NULL;
  }

  block_t *b = (block_t *)(uintptr_t)(heap_base + start);
  b->size = need;

  brk_offset = end;
  return (void *)((uint8_t *)b + hdr);
}

void kfree(void *ptr) {
  if (!ptr)
    return;
  const uint32_t hdr = header_size();
  block_t *b = (block_t *)((uint8_t *)ptr - hdr);
  b->next = free_list;
  free_list = b;
}
