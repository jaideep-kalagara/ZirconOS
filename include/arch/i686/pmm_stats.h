#pragma once
#include <stdint.h>

typedef struct {
  uint32_t total_frames_usable; // frames in [min, max)
  uint32_t used_frames_usable;  // frames handed out by PMM (total_alloc)
  uint32_t free_frames_usable;  // total - used

  uint32_t total_bytes_usable; // above * 4096
  uint32_t used_bytes_usable;
  uint32_t free_bytes_usable;

  uint32_t reserved_bytes_below_min; // frames < page_frame_min (kernel, boot)
  uint32_t total_bytes_physical;     // page_frame_max * 4096
  uint32_t used_bytes_overall;       // reserved_below_min + used_bytes_usable
} pmm_stats_t;

pmm_stats_t pmm_get_stats(void);
