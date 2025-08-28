#include "kernel/kinit.h"
#include "kernel/shell.h"

void kernel_main(uint32_t magic, void *boot_info_phys) {
  uint32_t mem_high_bytes;
  uint32_t physical_alloc_start;
  init(magic, boot_info_phys, &mem_high_bytes, &physical_alloc_start);

  loop(mem_high_bytes);
}
