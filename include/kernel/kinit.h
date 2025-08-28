#pragma once

#include <stdint.h>

void init(uint32_t magic, void *boot_info_phys, uint32_t *mem_high_bytes,
          uint32_t *physical_alloc_start);