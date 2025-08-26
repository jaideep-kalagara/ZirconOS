// arch/i686/irq.c
#include "arch/i686/irq.h"
#include "arch/i686/drivers/i8259.h"
#include "arch/i686/io.h"
#include "arch/i686/isr.h"
#include "arch/i686/pic.h"
#include "util/array.h"
#include <stdio.h>

#define PIC_REMAP_OFFSET 0x20

static irq_handler_t irq_handlers[16]; // zero-initialized

static const pic_driver *driver = NULL;

void i686_irq_handler(registers *regs) {
  int irq = regs->interrupt - PIC_REMAP_OFFSET;

  if (irq_handlers[irq] != NULL) {
    // handle IRQ
    irq_handlers[irq](regs);
  } else {
    printf("Unhandled IRQ %d...\n", irq);
  }

  // send EOI
  driver->send_end_of_interrupt(irq);
}

/* --- Public API --- */
void i686_init_irq() {
  const pic_driver *drivers[] = {
      i8259_get_driver(),

  };

  for (int i = 0; (signed)i < (int)SIZE(drivers); i++) {
    if (drivers[i]->probe()) {
      driver = drivers[i];
    }
  }

  if (driver == NULL) {
    printf("No PIC found!\n");
    return;
  }

  printf("Found %s driver.\n", driver->name);
  driver->initialize(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8, false);

  // register ISR handlers for each of the 16 irq lines
  for (int i = 0; i < 16; i++)
    i686_isr_register_handler(PIC_REMAP_OFFSET + i, i686_irq_handler);

  // enable interrupts
  i686_interrupts_enable();
}

void i686_irq_register_handler(int irq, irq_handler_t handler) {
  irq_handlers[irq] = handler;
}
