// Multiboot2 header (ELF kernel) — request memory info (required) + framebuffer
// (optional)
#include <stdint.h>

#define MB2_MAGIC 0xE85250D6u
#define MB2_ARCH 0u // i386

// ---- header-only tag structs (16-bit type/flags) ----
struct mb2_hdr_tag {
  uint16_t type, flags;
  uint32_t size;
} __attribute__((packed));

struct mb2_hdr_tag_info_req {
  uint16_t type, flags; // type=1
  uint32_t size;
  uint32_t requests[2]; // payload: list of requested *runtime* tag types
} __attribute__((packed));

struct mb2_hdr_tag_framebuffer {
  uint16_t type, flags; // type=5
  uint32_t size;
  uint32_t width, height, depth;
} __attribute__((packed));

// ---- whole header ----
struct mb2_header {
  uint32_t magic, architecture, header_length, checksum;

  struct mb2_hdr_tag_info_req info;  // type=1
  struct mb2_hdr_tag_framebuffer fb; // type=5 (optional)
  struct mb2_hdr_tag end;            // type=0
} __attribute__((packed, aligned(8)));

__attribute__((used, section(".multiboot2_header"), aligned(8)))
const struct mb2_header mb2_hdr = {
    .magic = MB2_MAGIC,
    .architecture = MB2_ARCH,
    .header_length = sizeof(struct mb2_header),
    .checksum =
        (uint32_t)(0 - (MB2_MAGIC + MB2_ARCH + sizeof(struct mb2_header))),

    // Information Request: REQUIRE meminfo (4) + mmap (6)
    .info =
        {
            .type = 1,
            .flags = 1, // required
            .size = sizeof(struct mb2_hdr_tag_info_req),
            .requests = {4u, 6u} // runtime tag types we want
        },

    .end = {.type = 0, .flags = 0, .size = 8}};
