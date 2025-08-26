// gdt.c
#include "arch/i686/gdt.h"
#include <arch/i686/io.h>
#include <stdint.h>

#define GDT_ENTRIES 3

// Each define here is for a specific flag in the descriptor.
// Refer to the intel documentation for a description of what each one does.
#define SEG_DESCTYPE(x)                                                        \
  ((x) << 0x04) // Descriptor type (0 for system, 1 for code/data)
#define SEG_PRES(x) ((x) << 0x07) // Present
#define SEG_SAVL(x) ((x) << 0x0C) // Available for system use
#define SEG_LONG(x) ((x) << 0x0D) // Long mode
#define SEG_SIZE(x) ((x) << 0x0E) // Size (0 for 16-bit, 1 for 32)
#define SEG_GRAN(x)                                                            \
  ((x) << 0x0F) // Granularity (0 for 1B - 1MB, 1 for 4KB - 4GB)
#define SEG_PRIV(x) (((x) & 0x03) << 0x05) // Set privilege level (0 - 3)

#define SEG_DATA_RD 0x00        // Read-Only
#define SEG_DATA_RDA 0x01       // Read-Only, accessed
#define SEG_DATA_RDWR 0x02      // Read/Write
#define SEG_DATA_RDWRA 0x03     // Read/Write, accessed
#define SEG_DATA_RDEXPD 0x04    // Read-Only, expand-down
#define SEG_DATA_RDEXPDA 0x05   // Read-Only, expand-down, accessed
#define SEG_DATA_RDWREXPD 0x06  // Read/Write, expand-down
#define SEG_DATA_RDWREXPDA 0x07 // Read/Write, expand-down, accessed
#define SEG_CODE_EX 0x08        // Execute-Only
#define SEG_CODE_EXA 0x09       // Execute-Only, accessed
#define SEG_CODE_EXRD 0x0A      // Execute/Read
#define SEG_CODE_EXRDA 0x0B     // Execute/Read, accessed
#define SEG_CODE_EXC 0x0C       // Execute-Only, conforming
#define SEG_CODE_EXCA 0x0D      // Execute-Only, conforming, accessed
#define SEG_CODE_EXRDC 0x0E     // Execute/Read, conforming
#define SEG_CODE_EXRDCA 0x0F    // Execute/Read, conforming, accessed

#define GDT_CODE_PL0                                                           \
  SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) | SEG_SIZE(1) |    \
      SEG_GRAN(1) | SEG_PRIV(0) | SEG_CODE_EXRD

#define GDT_DATA_PL0                                                           \
  SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) | SEG_SIZE(1) |    \
      SEG_GRAN(1) | SEG_PRIV(0) | SEG_DATA_RDWR

// ---- GDT storage ----
static uint64_t __attribute__((aligned(8))) gdt[GDT_ENTRIES];
struct __attribute__((packed)) gdtr {
  uint16_t limit;
  uint32_t base;
};
static struct gdtr gdtr;

// Encode your descriptor the same way you already did, but RETURN it
static inline uint64_t make_descriptor(uint32_t base, uint32_t limit,
                                       uint16_t flag) {
  uint64_t desc = 0;

  // high dword
  desc = (limit & 0x000F0000);                    // limit 19:16
  desc |= ((uint64_t)flag << 8) & 0x00F0FF00ULL;  // access + flags
  desc |= ((uint64_t)base >> 16) & 0x000000FFULL; // base 23:16
  desc |= ((uint64_t)base & 0xFF000000ULL);       // base 31:24

  desc <<= 32;

  // low dword
  desc |= ((uint64_t)base << 16) & 0xFFFF0000ULL; // base 15:0
  desc |= (uint64_t)(limit & 0x0000FFFF);         // limit 15:0

  return desc;
}

// Public: initialize + load GDT
void i686_init_gdt(void) {
  i686_interrupts_disable();

  // 0: null
  gdt[0] = 0;

  // 1: kernel code (base=0, limit=0xFFFFF with granularity=1 → 4GiB)
  gdt[1] = make_descriptor(0x00000000, 0x000FFFFF, GDT_CODE_PL0);

  // 2: kernel data (same span)
  gdt[2] = make_descriptor(0x00000000, 0x000FFFFF, GDT_DATA_PL0);

  // GDTR -> our table
  gdtr.limit = sizeof(gdt) - 1;
  gdtr.base = (uint32_t)gdt;

  // lgdt
  __asm__ volatile("lgdt %0" : : "m"(gdtr));

  // Reload segment registers. CS needs a far jump.
  __asm__ volatile("ljmp $0x08, $1f \n" // 0x08 = selector for gdt[1] (code)
                   "1:                    \n"
                   "mov $0x10, %%ax \n" // 0x10 = selector for gdt[2] (data)
                   "mov %%ax, %%ds   \n"
                   "mov %%ax, %%es   \n"
                   "mov %%ax, %%fs   \n"
                   "mov %%ax, %%gs   \n"
                   "mov %%ax, %%ss   \n"
                   :
                   :
                   : "ax", "memory");
}
