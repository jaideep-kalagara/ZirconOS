#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <arch/i686/io.h>
#include <arch/i686/pic.h>

#define PIC1_COMMAND_PORT 0x20
#define PIC1_DATA_PORT 0x21
#define PIC2_COMMAND_PORT 0xA0
#define PIC2_DATA_PORT 0xA1

enum {
  PIC_ICW1_ICW4 = 0x01,
  PIC_ICW1_SINGLE = 0x02,
  PIC_ICW1_INTERVAL4 = 0x04,
  PIC_ICW1_LEVEL = 0x08,
  PIC_ICW1_INITIALIZE = 0x10
} PIC_ICW1;

enum {
  PIC_ICW4_8086 = 0x1,
  PIC_ICW4_AUTO_EOI = 0x2,
  PIC_ICW4_BUFFER_MASTER = 0x4,
  PIC_ICW4_BUFFER_SLAVE = 0x0,
  PIC_ICW4_BUFFERRED = 0x8,
  PIC_ICW4_SFNM = 0x10,
} PIC_ICW4;

enum {
  PIC_CMD_END_OF_INTERRUPT = 0x20,
  PIC_CMD_READ_IRR = 0x0A,
  PIC_CMD_READ_ISR = 0x0B,
} PIC_CMD;

static uint16_t pic_mask = 0xffff;

void i8259_set_mask(uint16_t new_mask) {
  pic_mask = new_mask;
  i686_outb(PIC1_DATA_PORT, pic_mask & 0xFF);
  i686_iowait();
  i686_outb(PIC2_DATA_PORT, pic_mask >> 8);
  i686_iowait();
}

uint16_t i8259_get_mask() {
  return i686_inb(PIC1_DATA_PORT) | (i686_inb(PIC2_DATA_PORT) << 8);
}

void i8259_configure(uint8_t offset_pic1, uint8_t offset_pic2, bool auto_eoi) {
  // Mask everything
  i8259_set_mask(0xFFFF);

  // initialization control word 1
  i686_outb(PIC1_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
  i686_iowait();
  i686_outb(PIC2_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
  i686_iowait();

  // initialization control word 2 - the offsets
  i686_outb(PIC1_DATA_PORT, offset_pic1);
  i686_iowait();
  i686_outb(PIC2_DATA_PORT, offset_pic2);
  i686_iowait();

  // initialization control word 3
  i686_outb(PIC1_DATA_PORT,
            0x4); // tell PIC1 that it has a slave at IRQ2 (0000 0100)
  i686_iowait();
  i686_outb(PIC2_DATA_PORT, 0x2); // tell PIC2 its cascade identity (0000 0010)
  i686_iowait();

  // initialization control word 4
  uint8_t icw4 = PIC_ICW4_8086;
  if (auto_eoi) {
    icw4 |= PIC_ICW4_AUTO_EOI;
  }

  i686_outb(PIC1_DATA_PORT, icw4);
  i686_iowait();
  i686_outb(PIC2_DATA_PORT, icw4);
  i686_iowait();

  // mask all interrupts until they are enabled by the device driver
  i8259_set_mask(0xFFFF);
}

void i8259_send_end_of_interrupt(int irq) {
  if (irq >= 8)
    i686_outb(PIC2_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
  i686_outb(PIC1_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
}

void i8259_disable() { i8259_set_mask(0xFFFF); }

void i8259_mask(int irq) { i8259_set_mask(pic_mask | (1 << irq)); }

void i8259_unmask(int irq) { i8259_set_mask(pic_mask & ~(1 << irq)); }

uint16_t i8259_read_irq_request_register() {
  i686_outb(PIC1_COMMAND_PORT, PIC_CMD_READ_IRR);
  i686_outb(PIC2_COMMAND_PORT, PIC_CMD_READ_IRR);
  return ((uint16_t)i686_inb(PIC2_COMMAND_PORT)) |
         (((uint16_t)i686_inb(PIC2_COMMAND_PORT)) << 8);
}

uint16_t i8259_read_in_service_register() {
  i686_outb(PIC1_COMMAND_PORT, PIC_CMD_READ_ISR);
  i686_outb(PIC2_COMMAND_PORT, PIC_CMD_READ_ISR);
  return ((uint16_t)i686_inb(PIC2_COMMAND_PORT)) |
         (((uint16_t)i686_inb(PIC2_COMMAND_PORT)) << 8);
}

bool i8259_probe() {
  i8259_disable();
  i8259_set_mask(0x1337);
  return i8259_get_mask() == 0x1337;
}

static const pic_driver driver = {
    .name = "8259 PIC",
    .probe = &i8259_probe,
    .initialize = &i8259_configure,
    .disable = &i8259_disable,
    .send_end_of_interrupt = &i8259_send_end_of_interrupt,
    .mask = &i8259_mask,
    .unmask = &i8259_unmask,
};

const pic_driver *i8259_get_driver() { return &driver; }