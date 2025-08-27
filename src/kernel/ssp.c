#include <stdint.h>
#include <stdio.h>

#define STACK_CHK_GUARD 0xe2dee396

uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

__attribute__((noreturn)) void __stack_chk_fail(void) {
  printf("Stack smashing detected!");
  while (1)
    ;
}