#pragma once

#include <stdbool.h>
#include <stddef.h>

bool cpu_get_vendor(char vendor[13]); // "GenuineIntel", "AuthenticAMD", ...
bool cpu_get_brand_string(char *buf, size_t len);
