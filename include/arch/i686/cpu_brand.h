#ifndef _I686_CPU_BRAND_H
#define _I686_CPU_BRAND_H

#include <stdbool.h>
#include <stddef.h>

bool cpu_get_vendor(char vendor[13]); // "GenuineIntel", "AuthenticAMD", ...
bool cpu_get_brand_string(char *buf, size_t len);
#endif // _I686_CPU_BRAND_H