// multiboot2.h — minimal Multiboot2 header (8-byte aligned)
#include <stdint.h>

#define MB2_MAGIC 0xE85250D6u // Multiboot2 header magic
#define MB2_ARCH 0u           // 0 = i386

struct mb2_tag_header {
  uint16_t type;
  uint16_t flags;
  uint32_t size;
} __attribute__((packed)); // tag bodies follow; packed is fine here

struct mb2_header {
  uint32_t magic;            // MB2_MAGIC
  uint32_t architecture;     // MB2_ARCH
  uint32_t header_length;    // total size including all tags
  uint32_t checksum;         // -(magic + arch + header_length)
  struct mb2_tag_header end; // type=0, size=8
};

// compute length/checksum at compile time
#define MB2_HEADER_LEN (sizeof(struct mb2_header))
#define MB2_CHECKSUM (uint32_t)(0u - (MB2_MAGIC + MB2_ARCH + MB2_HEADER_LEN))

// Keep, place, and align the header
__attribute__((used, section(".multiboot2_header"), aligned(8)))
const struct mb2_header multiboot2_header = {
    .magic = MB2_MAGIC,
    .architecture = MB2_ARCH,
    .header_length = MB2_HEADER_LEN,
    .checksum = MB2_CHECKSUM,
    .end = {.type = 0, .flags = 0, .size = 8}};

_Static_assert((MB2_HEADER_LEN % 8) == 0,
               "MB2 header length must be multiple of 8");
