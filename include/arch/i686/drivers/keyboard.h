#pragma once

#include <arch/i686/irq.h>

void keyboard_init();
void keyboard_handler(registers *regs);

int keyboard_getchar(void);