#include "arch/i686/mm/paging.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define MAX_FRAMES (0x100000000u / 0x1000u)
#define BITMAP_BYTES (MAX_FRAMES / 8)

static uint8_t physical_memory_bitmap[BITMAP_BYTES];
static uint32_t frame_min,
    frame_max; // usable frame indices (inclusive/exclusive)

#define BIT_SET(b, i) ((b)[(i) >> 3] |= (1u << ((i) & 7)))
#define BIT_CLR(b, i) ((b)[(i) >> 3] &= ~(1u << ((i) & 7)))
#define BIT_TST(b, i) ((b)[(i) >> 3] & (1u << ((i) & 7)))

void pmm_init(uint32_t physical_alloc_start, uint32_t mem_high) {
  memset(physical_memory_bitmap, 0xFF,
         sizeof physical_memory_bitmap); // mark all used

  frame_min = (physical_alloc_start + 0xFFF) >> 12;
  frame_max =
      (mem_high & ~0xFFFu) >> 12; // mem_high = top usable PA (exclusive)

  // Mark usable frames [frame_min, frame_max) as free
  for (uint32_t f = frame_min; f < frame_max; ++f)
    BIT_CLR(physical_memory_bitmap, f);
}

uint32_t pmm_alloc_page(void) { // returns PA of a 4KiB page, or 0
  for (uint32_t f = frame_min; f < frame_max; ++f) {
    if (!BIT_TST(physical_memory_bitmap, f)) {
      BIT_SET(physical_memory_bitmap, f);
      return f << 12;
    }
  }
  return 0;
}

void pmm_free_page(uint32_t pa) {
  uint32_t f = pa >> 12;
  if (f >= frame_min && f < frame_max)
    BIT_CLR(physical_memory_bitmap, f);
}

volatile uint32_t *paging_ensure_pt(uint32_t va, uint32_t pde_flags) {
  uint32_t pdi = PD_INDEX(va);
  volatile uint32_t *pd = PD_VIRT;

  if (!(pd[pdi] & PG_P)) {
    uint32_t pt_pa = pmm_alloc_page();
    if (!pt_pa)
      return 0;

    pd[pdi] = (pt_pa & ~0xFFFu) | PG_P | PG_RW | (pde_flags & (PG_US));
    // PT is visible through self-ref window; zero it
    invlpg(PT_BASE_VIRT + (pdi << 12));
    memset((void *)PT_VIRT(pdi), 0, 4096);
  }
  return PT_VIRT(pdi);
}

bool paging_map(uint32_t va, uint32_t pa, uint32_t flags) {
  volatile uint32_t *pt = paging_ensure_pt(va, 0);
  if (!pt)
    return false;

  uint32_t pti = PT_INDEX(va);
  if (pt[pti] & PG_P)
    return true; // already mapped (or change flags if you prefer)
  pt[pti] = (pa & ~0xFFFu) | (flags & 0xFFFu) | PG_P;
  invlpg(va);
  return true;
}

void paging_unmap(uint32_t va) {
  volatile uint32_t *pt = paging_ensure_pt(va, 0);
  if (!pt)
    return;
  pt[PT_INDEX(va)] = 0;
  invlpg(va);
}
