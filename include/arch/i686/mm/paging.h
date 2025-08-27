// paging.h
#pragma once
#include <stdbool.h>
#include <stdint.h>

#define PG_P 0x001
#define PG_RW 0x002
#define PG_US 0x004
#define PG_PS 0x080

#define PD_SELF_INDEX 1023
#define PD_VIRT ((volatile uint32_t *)0xFFFFF000)
#define PT_BASE_VIRT 0xFFC00000u
#define PT_VIRT(i) ((volatile uint32_t *)(PT_BASE_VIRT + ((i) << 12)))

#define PD_INDEX(va) (((va) >> 22) & 0x3FF)
#define PT_INDEX(va) (((va) >> 12) & 0x3FF)

void pmm_init(uint32_t physical_alloc_start, uint32_t mem_high);
uint32_t pmm_alloc_page(void);
void pmm_free_page(uint32_t pa);

// TLB helpers
static inline void invlpg(uint32_t va) {
  __asm__ volatile("invlpg (%0)" ::"r"(va) : "memory");
}
static inline uint32_t rdcr3(void) {
  uint32_t v;
  __asm__ volatile("mov %%cr3,%0" : "=r"(v));
  return v;
}
static inline void wrcr3(uint32_t v) {
  __asm__ volatile("mov %0,%%cr3" ::"r"(v) : "memory");
}

// call once after paging is active and you know PD PA
static inline void paging_install_selfref(uint32_t pd_pa) {
  volatile uint32_t *pd = PD_VIRT; // assumes PD is already virtually accessible
  pd[PD_SELF_INDEX] = (pd_pa & ~0xFFFu) | PG_P | PG_RW;
  wrcr3(pd_pa); // activate PD/PT windows (flush)
}

volatile uint32_t *paging_ensure_pt(uint32_t va, uint32_t pde_flags);
bool paging_map(uint32_t va, uint32_t pa, uint32_t flags);
void paging_unmap(uint32_t va);
