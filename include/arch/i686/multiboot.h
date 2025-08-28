// multiboot2.h — minimal, practical MB2 definitions + helpers
#pragma once
#include <stdint.h>

// ---- Bootloader magic (EAX at entry) ----
#define MB2_BOOTLOADER_MAGIC 0x36D76289u

// ---- Info structure header (pointed to by EBX; PHYSICAL address) ----
// Layout: u32 total_size; u32 reserved; then a sequence of tags, 8-byte
// aligned.
struct mb2_info_fixed {
  uint32_t total_size;
  uint32_t reserved;
};

// ---- Generic tag header ----
struct mb2_tag {
  uint32_t type; // 0=end, 1=cmdline, 2=boot loader name, 3=module, 4=basic mem,
                 // 6=mmap, 8=framebuffer, ...
  uint32_t size; // total bytes for this tag including this header
} __attribute__((packed));

// ---- Tag types (subset we use) ----
enum {
  MB2_TAG_END = 0,
  MB2_TAG_CMDLINE = 1,
  MB2_TAG_BOOT_LOADER_NAME = 2,
  MB2_TAG_MODULE = 3,
  MB2_TAG_BASIC_MEMINFO = 4, // struct below
  MB2_TAG_BOOTDEV = 5,
  MB2_TAG_MMAP = 6, // struct below
  MB2_TAG_VBE = 7,
  MB2_TAG_FRAMEBUFFER = 8, // struct below
  MB2_TAG_ELF_SECTIONS = 9,
  MB2_TAG_APM = 10,
};

// ---- Basic memory info (type 4) ----
// mem_lower/mem_upper are in KiB (like MB1), mem_upper is above 1 MiB.
struct mb2_tag_basic_meminfo {
  uint32_t type;      // = 4
  uint32_t size;      // = 16
  uint32_t mem_lower; // KiB below 1 MiB
  uint32_t mem_upper; // KiB above 1 MiB
} __attribute__((packed));

// ---- Memory map (type 6) ----
struct mb2_tag_mmap {
  uint32_t type; // = 6
  uint32_t size; // header + entries
  uint32_t entry_size;
  uint32_t entry_version;
  // then entries[]
} __attribute__((packed));

struct mb2_mmap_entry {
  uint64_t addr; // base (physical)
  uint64_t len;  // length in bytes
  uint32_t type; // 1=available, 2=reserved, 3=ACPI, 4=NVS, 5=badram
  uint32_t zero;
} __attribute__((packed));

// ---- Framebuffer (type 8) ----
struct mb2_tag_framebuffer {
  uint32_t type;             // = 8
  uint32_t size;             // >= 32
  uint64_t framebuffer_addr; // physical
  uint32_t framebuffer_pitch;
  uint32_t framebuffer_width;
  uint32_t framebuffer_height;
  uint8_t framebuffer_bpp;
  uint8_t framebuffer_type; // 0=indexed, 1=RGB, 2=EGA text
  uint8_t reserved;
  // followed by palette or RGB field info depending on type (not needed here)
} __attribute__((packed));

// ---- Alignment helper for walking tags ----
static inline uint32_t mb2_align8(uint32_t x) { return (x + 7u) & ~7u; }

// ---- Iterate next tag given current ----
static inline const struct mb2_tag *mb2_next_tag(const struct mb2_tag *t) {
  return (const struct mb2_tag *)((const uint8_t *)t + mb2_align8(t->size));
}

// ---- Find first tag of a given type (info = VA of mb2_info_fixed) ----
static inline const struct mb2_tag *
mb2_find_tag(const struct mb2_info_fixed *info, uint32_t type) {
  const uint8_t *base = (const uint8_t *)info;
  const uint8_t *end = base + info->total_size;
  const struct mb2_tag *t = (const struct mb2_tag *)(base + 8); // skip header
  while ((const uint8_t *)t < end && t->type != MB2_TAG_END) {
    if (t->type == type)
      return t;
    t = mb2_next_tag(t);
  }
  return NULL;
}

// ---- Compute top-of-RAM in BYTES (prefer E820; fallback to basic mem info)
// ----
static inline uint32_t mb2_mem_top_bytes(const struct mb2_info_fixed *info) {
  // Try E820 mmap
  const struct mb2_tag_mmap *mm =
      (const struct mb2_tag_mmap *)mb2_find_tag(info, MB2_TAG_MMAP);
  uint64_t top = 0;
  if (mm) {
    const uint8_t *p = (const uint8_t *)mm + sizeof(*mm);
    const uint8_t *e = (const uint8_t *)mm + mm->size;
    while (p + mm->entry_size <= e) {
      const struct mb2_mmap_entry *me = (const struct mb2_mmap_entry *)p;
      if (me->type == 1) { // available
        uint64_t end = me->addr + me->len;
        if (end > top)
          top = end;
      }
      p += mm->entry_size;
    }
    if (top > 0xFFFFFFFFull)
      top = 0xFFFFFFFFull; // clamp to 32-bit
    if (top)
      return (uint32_t)top;
  }

  // Fallback to basic mem info
  const struct mb2_tag_basic_meminfo *bi =
      (const struct mb2_tag_basic_meminfo *)mb2_find_tag(info,
                                                         MB2_TAG_BASIC_MEMINFO);
  if (bi) {
    // 1 MiB + upper(KiB)*1024
    return 0x00100000u + bi->mem_upper * 1024u;
  }

  // Worst-case: assume at least 1 MiB
  return 0x00100000u;
}

// ---- Fetch framebuffer parameters if present; returns 1 if ok, 0 otherwise
// ----
static inline int mb2_get_framebuffer(const struct mb2_info_fixed *info,
                                      uint64_t *phys_addr, uint32_t *pitch,
                                      uint32_t *w, uint32_t *h, uint8_t *bpp,
                                      uint8_t *type) {
  const struct mb2_tag_framebuffer *fb =
      (const struct mb2_tag_framebuffer *)mb2_find_tag(info,
                                                       MB2_TAG_FRAMEBUFFER);
  if (!fb)
    return 0;
  if (phys_addr)
    *phys_addr = fb->framebuffer_addr;
  if (pitch)
    *pitch = fb->framebuffer_pitch;
  if (w)
    *w = fb->framebuffer_width;
  if (h)
    *h = fb->framebuffer_height;
  if (bpp)
    *bpp = fb->framebuffer_bpp;
  if (type)
    *type = fb->framebuffer_type;
  return 1;
}
