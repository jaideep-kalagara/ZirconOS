#pragma once

#include <stdint.h>

void analyze_cmd(char *cmd, uint32_t mem_high_bytes);
void loop(uint32_t mem_high_bytes);