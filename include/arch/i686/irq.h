#pragma once
#include "isr.h"

typedef void (*irq_handler_t)(registers *regs);

void i686_init_irq();
void i686_irq_register_handler(int irq, irq_handler_t handler);