// Multiboot2 header (minimal) — keep in low .boot
#include <stdint.h>

#define MB2_MAGIC 0xE85250D6u
#define MB2_ARCH 0u

struct mb2_tag_header {
  uint16_t type, flags;
  uint32_t size;
} __attribute__((packed));
struct mb2_header {
  uint32_t magic, architecture, header_length, checksum;
  struct mb2_tag_header end;
};

#define MB2_LEN (sizeof(struct mb2_header))
#define MB2_CHK (uint32_t)(0u - (MB2_MAGIC + MB2_ARCH + MB2_LEN))

__attribute__((used, section(".multiboot2_header"), aligned(8)))
const struct mb2_header multiboot2_header = {
    .magic = MB2_MAGIC,
    .architecture = MB2_ARCH,
    .header_length = MB2_LEN,
    .checksum = MB2_CHK,
    .end = {.type = 0, .flags = 0, .size = 8}};
